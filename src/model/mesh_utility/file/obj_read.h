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
        std::optional<T> t;

        read_float(str, v, &t);

        if (t && *t != 0)
        {
                error(to_string(N + 1) + "-dimensional textures are not supported");
        }
}

inline void read_library_names(
        const char* begin,
        const char* const end,
        std::vector<std::filesystem::path>* const library_names,
        std::set<std::filesystem::path>* const unique_library_names)
{
        bool found = false;

        while (true)
        {
                begin = read(begin, end, ascii::is_space);

                if (begin == end)
                {
                        if (!found)
                        {
                                error("Library name not found");
                        }
                        return;
                }

                const char* const iter = begin;
                begin = read(begin, end, ascii::is_not_space);
                std::filesystem::path name = path_from_utf8(std::string(iter, begin));
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
