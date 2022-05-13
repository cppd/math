/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "../../../mesh.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/read.h>
#include <src/com/string/ascii.h>

#include <array>

namespace ns::model::mesh::file::obj
{
namespace facet_implementation
{
// "x/x/x"
// "x//x"
// "x//"
// "x/x/"
// "x/x"
// "x"
template <std::size_t GROUP_SIZE, typename T>
const char* read_digit_group(const char* first, const char* const last, std::array<T, GROUP_SIZE>* const group_indices)
{
        static_assert(std::is_integral_v<T> && std::is_signed_v<T>);

        // vertex
        {
                const auto [value, ptr] = read_from_chars<T>(first, last);
                if (value)
                {
                        if (*value == 0)
                        {
                                error("Zero facet index");
                        }
                        (*group_indices)[0] = *value;
                        first = ptr;
                }
                else
                {
                        error("Error read facet vertex first number");
                }
        }

        // texture and normal
        for (unsigned a = 1; a < group_indices->size(); ++a)
        {
                if (first == last || ascii::is_space(*first))
                {
                        (*group_indices)[a] = 0;
                        continue;
                }

                if (*first != '/')
                {
                        error(std::string("Error read facet number, expected '/', found '") + *first + "'");
                }

                ++first;

                if (first == last || ascii::is_space(*first))
                {
                        (*group_indices)[a] = 0;
                        continue;
                }

                const auto [value, ptr] = read_from_chars<T>(first, last);
                if (value)
                {
                        if (*value == 0)
                        {
                                error("Zero facet index");
                        }
                        (*group_indices)[a] = *value;
                        first = ptr;
                }
                else
                {
                        (*group_indices)[a] = 0;
                }
        }

        return first;
}

template <std::size_t MAX_GROUP_COUNT, std::size_t GROUP_SIZE, typename IndexType>
void read_digit_groups(
        const char* first,
        const char* const last,
        std::array<std::array<IndexType, GROUP_SIZE>, MAX_GROUP_COUNT>* const group_ptr,
        int* const group_count)
{
        unsigned group_index = -1;

        while (true)
        {
                ++group_index;

                first = read(first, last, ascii::is_space);

                if (first == last)
                {
                        *group_count = group_index;
                        return;
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

        int group_count;

        impl::read_digit_groups(first, last, &groups, &group_count);

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

                for (unsigned n = 1; n < N; ++n)
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
        for (std::size_t i = 0; i < N; ++i)
        {
                int& v = facet->vertices[i];
                int& t = facet->texcoords[i];
                int& n = facet->normals[i];

                if (v == 0)
                {
                        error("Correct indices vertex index is zero");
                }

                v = v > 0 ? v - 1 : vertices_size + v;
                t = t > 0 ? t - 1 : (t < 0 ? texcoords_size + t : -1);
                n = n > 0 ? n - 1 : (n < 0 ? normals_size + n : -1);
        }
}
}
