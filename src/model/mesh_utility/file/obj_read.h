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

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/com/type/name.h>
#include <src/model/mesh.h>
#include <src/numerical/vector.h>

#include <filesystem>
#include <set>
#include <vector>

namespace ns::mesh::file
{
template <std::size_t N, typename T>
void read_float_texture(const char* const str, Vector<N, T>* const v)
{
        T tmp;

        const int n = read_float(str, v, &tmp).first;

        if (n != N && n != N + 1)
        {
                error("Error read " + to_string(N) + " or " + to_string(N + 1) + " floating points of " + type_name<T>()
                      + " type");
        }

        if (n == N + 1 && tmp != 0)
        {
                error(to_string(N + 1) + "-dimensional textures are not supported");
        }
}

template <typename T>
void read_library_names(
        const T& data,
        const long long begin,
        const long long end,
        std::vector<std::filesystem::path>* const library_names,
        std::set<std::filesystem::path>* const unique_library_names)
{
        const long long size = end;

        bool found = false;
        long long i = begin;

        while (true)
        {
                read(data, size, ascii::is_space, &i);

                if (i == size)
                {
                        if (!found)
                        {
                                error("Library name not found");
                        }
                        return;
                }

                long long i2 = i;
                read(data, size, ascii::is_not_space, &i2);
                std::filesystem::path name = path_from_utf8(std::string(&data[i], i2 - i));
                i = i2;
                found = true;

                if (!unique_library_names->contains(name))
                {
                        library_names->push_back(name);
                        unique_library_names->insert(std::move(name));
                }
        }
}

// Positive OBJ indices indicate absolute vertex numbers.
// Negative OBJ indices indicate relative vertex numbers.
// Convert to absolute numbers starting at 0.
template <std::size_t N>
void correct_indices(
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
