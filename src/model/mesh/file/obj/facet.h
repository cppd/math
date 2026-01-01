/*
Copyright (C) 2017-2026 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/read.h>
#include <src/com/string/ascii.h>
#include <src/model/mesh.h>
#include <src/model/mesh/file/data_read.h>

#include <array>
#include <cstddef>
#include <string>

namespace ns::model::mesh::file::obj
{
namespace facet_implementation
{
template <typename T>
[[nodiscard]] const char* read_first_index(const char* const first, const char* const last, T* const index)
{
        static_assert(std::is_integral_v<T> && std::is_signed_v<T>);

        const auto [value, ptr] = read_from_chars<T>(first, last);

        if (!value)
        {
                error("Error read facet vertex first number");
        }

        if (*value == 0)
        {
                error("Zero facet index");
        }

        *index = *value;
        return ptr;
}

template <typename T>
[[nodiscard]] const char* read_optional_index(const char* first, const char* const last, T* const index)
{
        static_assert(std::is_integral_v<T> && std::is_signed_v<T>);

        if (first == last || ascii::is_space(*first))
        {
                *index = 0;
                return first;
        }

        if (*first != '/')
        {
                error(std::string("Error read facet number, expected '/', found '") + *first + "'");
        }

        ++first;

        if (first == last || ascii::is_space(*first))
        {
                *index = 0;
                return first;
        }

        const auto [value, ptr] = read_from_chars<T>(first, last);

        if (!value)
        {
                *index = 0;
                return first;
        }

        if (*value == 0)
        {
                error("Zero facet index");
        }

        *index = *value;
        return ptr;
}

// "x/x/x"
// "x//x"
// "x//"
// "x/x/"
// "x/x"
// "x"
template <std::size_t GROUP_SIZE, typename T>
[[nodiscard]] const char* read_digit_group(
        const char* first,
        const char* const last,
        std::array<T, GROUP_SIZE>* const group_indices)
{
        static_assert(GROUP_SIZE > 0);

        first = read_first_index(first, last, &(*group_indices)[0]);

        for (std::size_t i = 1; i < group_indices->size(); ++i)
        {
                first = read_optional_index(first, last, &(*group_indices)[i]);
        }

        return first;
}

template <std::size_t MAX_GROUP_COUNT, std::size_t GROUP_SIZE, typename IndexType>
[[nodiscard]] unsigned read_digit_groups(
        const char* first,
        const char* const last,
        std::array<std::array<IndexType, GROUP_SIZE>, MAX_GROUP_COUNT>* const group_ptr)
{
        unsigned group_index = -1;

        while (true)
        {
                ++group_index;

                first = read(first, last, ascii::is_space);

                if (first == last)
                {
                        return group_index;
                }

                if (group_index >= group_ptr->size())
                {
                        error("Found too many facet vertices " + to_string(group_index + 1)
                              + " (max supported = " + to_string(group_ptr->size()) + ")");
                }

                first = read_digit_group(first, last, &(*group_ptr)[group_index]);
        }
}

template <typename T, std::size_t MAX_GROUP_COUNT>
void check_index_consistency(const std::array<std::array<T, 3>, MAX_GROUP_COUNT>& groups, const int group_count)
{
        // 0 means there is no index.
        // index order: facet, texture, normal.

        ASSERT(group_count <= static_cast<int>(groups.size()));

        int texture = 0;
        int normal = 0;

        for (int i = 0; i < group_count; ++i)
        {
                texture += (groups[i][1] != 0) ? 1 : 0;
                normal += (groups[i][2] != 0) ? 1 : 0;
        }

        if (!(texture == 0 || texture == group_count))
        {
                error("Inconsistent facet texture indices");
        }

        if (!(normal == 0 || normal == group_count))
        {
                error("Inconsistent facet normal indices");
        }
}
}

template <std::size_t N, std::size_t MAX_FACETS>
void read_facets(
        const char* const first,
        const char* const last,
        std::array<typename Mesh<N>::Facet, MAX_FACETS>* const facets,
        int* const facet_count)
{
        static_assert(N >= 3);

        namespace impl = facet_implementation;

        constexpr int MAX_GROUP_COUNT = MAX_FACETS + N - 1;

        std::array<std::array<int, 3>, MAX_GROUP_COUNT> groups;

        const unsigned group_count = impl::read_digit_groups(first, last, &groups);

        if (group_count < static_cast<int>(N))
        {
                error("Error facet vertex count " + to_string(group_count) + " (min = " + to_string(N) + ")");
        }

        impl::check_index_consistency(groups, group_count);

        *facet_count = group_count - (N - 1);

        for (int i = 0; i < *facet_count; ++i)
        {
                (*facets)[i].has_texcoord = !(groups[0][1] == 0);
                (*facets)[i].has_normal = !(groups[0][2] == 0);

                (*facets)[i].vertices[0] = groups[0][0];
                (*facets)[i].texcoords[0] = groups[0][1];
                (*facets)[i].normals[0] = groups[0][2];

                for (std::size_t n = 1; n < N; ++n)
                {
                        (*facets)[i].vertices[n] = groups[i + n][0];
                        (*facets)[i].texcoords[n] = groups[i + n][1];
                        (*facets)[i].normals[n] = groups[i + n][2];
                }
        }
}

// Positive OBJ indices indicate absolute vertex numbers.
// Negative OBJ indices indicate relative vertex numbers.
// Convert to absolute numbers starting at 0.
template <std::size_t N>
void correct_facet_indices(
        typename Mesh<N>::Facet* const facet,
        const int vertices_size,
        const int texcoords_size,
        const int normals_size)
{
        const auto correct_vertex_index = [&](const int index)
        {
                if (index > 0)
                {
                        return index - 1;
                }
                if (index < 0)
                {
                        return vertices_size + index;
                }
                error("Correct facet indices, vertex index is zero");
        };

        const auto correct_index = [](const int index, const int size)
        {
                if (index > 0)
                {
                        return index - 1;
                }
                if (index < 0)
                {
                        return size + index;
                }
                return -1;
        };

        for (std::size_t i = 0; i < N; ++i)
        {
                facet->vertices[i] = correct_vertex_index(facet->vertices[i]);
                facet->texcoords[i] = correct_index(facet->texcoords[i], texcoords_size);
                facet->normals[i] = correct_index(facet->normals[i], normals_size);
        }
}
}
