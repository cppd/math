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

#include "data_read.h"

#include "../../mesh.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/string/ascii.h>

#include <array>

namespace ns::mesh::file
{
namespace obj_facet_implementation
{
// "x/x/x"
// "x//x"
// "x//"
// "x/x/"
// "x/x"
// "x"
template <typename T, std::size_t GROUP_SIZE, typename IndexType>
void read_digit_group(
        const T& line,
        const long long end,
        long long* const from,
        std::array<IndexType, GROUP_SIZE>* const group_indices)
{
        // vertex
        if (read_integer(line, end, from, &(*group_indices)[0]))
        {
                if ((*group_indices)[0] == 0)
                {
                        error("Zero facet index");
                }
        }
        else
        {
                error("Error read facet vertex first number");
        }

        // texture and normal
        for (int a = 1; a < static_cast<int>(group_indices->size()); ++a)
        {
                if (*from == end || ascii::is_space(line[*from]))
                {
                        (*group_indices)[a] = 0;
                        continue;
                }

                if (line[*from] != '/')
                {
                        error(std::string("Error read facet number, expected '/', found '") + line[*from] + "'");
                }

                ++(*from);

                if (*from == end || ascii::is_space(line[*from]))
                {
                        (*group_indices)[a] = 0;
                        continue;
                }

                if (read_integer(line, end, from, &(*group_indices)[a]))
                {
                        if ((*group_indices)[a] == 0)
                        {
                                error("Zero facet index");
                        }
                }
                else
                {
                        (*group_indices)[a] = 0;
                }
        }
}

template <typename T, std::size_t MAX_GROUP_COUNT, std::size_t GROUP_SIZE, typename IndexType>
void read_digit_groups(
        const T& line,
        const long long begin,
        const long long end,
        std::array<std::array<IndexType, GROUP_SIZE>, MAX_GROUP_COUNT>* const group_ptr,
        int* const group_count)
{
        int group_index = -1;

        long long i = begin;

        while (true)
        {
                ++group_index;

                read(line, end, ascii::is_space, &i);

                if (i == end)
                {
                        *group_count = group_index;
                        return;
                }

                if (group_index >= static_cast<int>(group_ptr->size()))
                {
                        error("Found too many facet vertices " + to_string(group_index + 1)
                              + " (max supported = " + to_string(group_ptr->size()) + ")");
                }

                read_digit_group(line, end, &i, &(*group_ptr)[group_index]);
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

template <std::size_t N, typename T, std::size_t MAX_FACETS>
void read_facets(
        const T& data,
        const long long begin,
        const long long end,
        std::array<typename Mesh<N>::Facet, MAX_FACETS>* const facets,
        int* const facet_count)
{
        static_assert(N >= 3);

        namespace impl = obj_facet_implementation;

        constexpr int MAX_GROUP_COUNT = MAX_FACETS + N - 1;

        std::array<std::array<int, 3>, MAX_GROUP_COUNT> groups;

        int group_count;

        impl::read_digit_groups(data, begin, end, &groups, &group_count);

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
}
