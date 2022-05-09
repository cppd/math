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
#include <src/com/string/ascii.h>
#include <src/numerical/vector.h>

#include <filesystem>
#include <set>
#include <vector>

namespace ns::mesh::file::obj
{
namespace data_read_implementation
{
struct Split final
{
        long long first_b;
        long long first_e;
        long long second_b;
        long long second_e;
};

// split string into two parts
// 1. not space characters
// 2. all other characters before a comment or the end of the string
inline Split split(const std::vector<char>& data, const long long first, const long long last)
{
        const auto is_comment = [](char c)
        {
                return c == '#';
        };

        long long i = first;

        while (i < last && ascii::is_space(data[i]))
        {
                ++i;
        }

        if (i == last || is_comment(data[i]))
        {
                return {.first_b = i, .first_e = i, .second_b = i, .second_e = i};
        }

        long long i2 = i + 1;
        while (i2 < last && !ascii::is_space(data[i2]) && !is_comment(data[i2]))
        {
                ++i2;
        }

        Split split;

        split.first_b = i;
        split.first_e = i2;

        i = i2;

        if (i == last || is_comment(data[i]))
        {
                split.second_b = i;
                split.second_e = i;
                return split;
        }

        // skip the first space
        ++i;

        i2 = i;
        while (i2 < last && !is_comment(data[i2]))
        {
                ++i2;
        }

        split.second_b = i;
        split.second_e = i2;
        return split;
}
}

struct SplitLine final
{
        std::string_view first;
        const char* second_b;
        const char* second_e;
};

inline SplitLine split_line(
        const std::vector<char>& data,
        const std::vector<long long>& line_begin,
        const long long line)
{
        namespace impl = data_read_implementation;

        const long long line_count = line_begin.size();

        const long long last = ((line + 1 < line_count) ? line_begin[line + 1] : data.size()) - 1;

        ASSERT(last >= 0 && !data[last]);

        const impl::Split split = impl::split(data, line_begin[line], last);

        return {
                .first = {&data[split.first_b], &data[split.first_e]},
                .second_b = &data[split.second_b],
                .second_e = &data[split.second_e]
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
