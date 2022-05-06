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

#include "../data_read.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/numerical/vector.h>

#include <filesystem>
#include <set>
#include <vector>

namespace ns::mesh::file::obj
{
template <std::size_t N, typename T>
void read_float_texture(const char* const str, Vector<N, T>* const v)
{
        std::optional<T> t;

        read_float(str, v, &t);

        if (t && *t != 0)
        {
                error(std::to_string(N + 1) + "-dimensional textures are not supported");
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
}
