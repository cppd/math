/*
Copyright (C) 2017-2025 Topological Manifold

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
#include <src/com/file/path.h>
#include <src/com/string/ascii.h>
#include <src/model/mesh/file/data_read.h>

#include <array>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ns::model::mesh::file::obj
{
struct Split final
{
        std::string_view first;
        const char* second_b;
        const char* second_e;
};

// split string into two parts
// 1. not space characters
// 2. all other characters before a comment or the end of the string
inline Split split_string(const std::array<const char*, 2> str)
{
        const char* const first = str[0];
        const char* const last = str[1];

        const auto is_comment = [](const char c)
        {
                return c == '#';
        };

        const char* i = first;

        while (i < last && ascii::is_space(*i))
        {
                ++i;
        }

        if (i == last || is_comment(*i))
        {
                return {
                        .first = {i, i},
                        .second_b = i,
                        .second_e = i
                };
        }

        const char* i2 = i + 1;
        while (i2 < last && !ascii::is_space(*i2) && !is_comment(*i2))
        {
                ++i2;
        }

        const std::string_view split_first{i, i2};

        i = i2;

        if (i == last || is_comment(*i))
        {
                return {
                        .first = split_first,
                        .second_b = i,
                        .second_e = i,
                };
        }

        // skip the first space
        ++i;

        i2 = i;
        while (i2 < last && !is_comment(*i2))
        {
                ++i2;
        }

        return {
                .first = split_first,
                .second_b = i,
                .second_e = i2,
        };
}

inline std::string_view read_name(const std::string_view object_name, const char* const first, const char* const last)
{
        const char* const i1 = read(first, last, ascii::is_space);
        if (i1 == last)
        {
                error("Error read " + std::string(object_name) + " name");
        }

        const char* const i2 = read(i1, last, ascii::is_not_space);
        if (i2 == i1)
        {
                error("Error read " + std::string(object_name) + " name");
        }

        const char* const i3 = read(i2, last, ascii::is_space);
        if (i3 != last)
        {
                error("Error read " + std::string(object_name) + " name");
        }

        return {i1, i2};
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
                std::filesystem::path name = path_from_utf8(std::string_view(iter, begin));
                found = true;

                if (!unique_library_names->contains(name))
                {
                        library_names->push_back(name);
                        unique_library_names->insert(std::move(name));
                }
        }
}
}
