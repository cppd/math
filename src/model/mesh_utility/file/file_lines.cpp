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
Lines::Lines(std::vector<char>&& text_data) : data_(std::move(text_data))
{
        if (data_.empty())
        {
                return;
        }

        if (data_.back() != '\n')
        {
                data_.push_back('\n');
        }

        const long long line_count = [&]
        {
                long long res = 0;
                for (const char c : data_)
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

        beginnings_.resize(line_count);

        long long beginning = 0;
        long long line = 0;
        for (long long i = 0, size = data_.size(); i < size; ++i)
        {
                if (data_[i] != '\n')
                {
                        continue;
                }
                data_[i] = '\0';
                beginnings_[line++] = beginning;
                beginning = i + 1;
        }
        ASSERT(data_.back() == '\0');
        ASSERT(line == line_count);
}
}
