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

#include "data_read.h"

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
namespace
{
[[nodiscard]] bool is_comment(const char c)
{
        return c == '#';
}

[[nodiscard]] const char* skip_space(const char* index, const char* const last)
{
        while (index < last && ascii::is_space(*index))
        {
                ++index;
        }
        return index;
}

[[nodiscard]] const char* skip_not_space(const char* index, const char* const last)
{
        while (index < last && !ascii::is_space(*index) && !is_comment(*index))
        {
                ++index;
        }
        return index;
}

[[nodiscard]] const char* skip_not_comment(const char* index, const char* const last)
{
        while (index < last && !is_comment(*index))
        {
                ++index;
        }
        return index;
}
}

// split string into two parts
// 1. not space characters
// 2. all other characters before a comment or the end of the string
Split split_string(const std::array<const char*, 2> str)
{
        const char* const first = str[0];
        const char* const last = str[1];

        const char* const i1 = skip_space(first, last);

        if (i1 == last || is_comment(*i1))
        {
                return {
                        .first = {i1, i1},
                        .second_b = i1,
                        .second_e = i1,
                };
        }

        const char* const i2 = skip_not_space(i1 + 1, last);

        const std::string_view split_first{i1, i2};

        if (i2 == last || is_comment(*i2))
        {
                return {
                        .first = split_first,
                        .second_b = i2,
                        .second_e = i2,
                };
        }

        // skip the first space
        const char* const i3 = i2 + 1;

        const char* const i4 = skip_not_comment(i3, last);

        return {
                .first = split_first,
                .second_b = i3,
                .second_e = i4,
        };
}

std::string_view read_name(const std::string_view object_name, const char* const first, const char* const last)
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

void read_library_names(
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
