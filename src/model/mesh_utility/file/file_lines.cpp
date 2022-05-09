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

#include "file_lines.h"

#include <src/com/error.h>

namespace ns::mesh::file
{
Lines make_lines(std::vector<char>&& text_data)
{
        if (text_data.empty())
        {
                return {};
        }

        if (text_data.back() != '\n')
        {
                text_data.push_back('\n');
        }

        const long long line_count = [&]
        {
                long long res = 0;
                for (const char c : text_data)
                {
                        if (!c)
                        {
                                error("Text data contains null character");
                        }
                        if (c == '\n')
                        {
                                ++res;
                        }
                }
                return res;
        }();

        std::vector<long long> beginnings(line_count);

        long long beginning = 0;
        std::size_t line = 0;
        for (long long i = 0, size = text_data.size(); i < size; ++i)
        {
                if (text_data[i] == '\n')
                {
                        text_data[i] = '\0';
                        beginnings[line++] = beginning;
                        beginning = i + 1;
                }
        }
        ASSERT(text_data.back() == '\0');

        return {.data = std::move(text_data), .beginnings = std::move(beginnings)};
}
}
