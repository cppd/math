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

#include "lines.h"

#include <src/com/error.h>

#include <utility>
#include <vector>

namespace ns::model::mesh::file
{
namespace
{
long long make_lines(std::vector<char>* const data)
{
        ASSERT(!data->empty() && data->back() == '\n');

        long long count = 0;
        for (char& c : *data)
        {
                if (!c)
                {
                        error("Text data contains null character");
                }

                if (c == '\n')
                {
                        ++count;
                        c = '\0';
                }
        }
        return count;
}
}

Lines::Lines(std::vector<char>&& text_data)
        : data_(std::move(text_data))
{
        if (data_.empty())
        {
                return;
        }

        if (data_.back() != '\n')
        {
                data_.push_back('\n');
        }

        const unsigned long long line_count = make_lines(&data_);

        lines_.reserve(line_count);

        const char* beginning = data_.data();
        const char* const last = data_.data() + data_.size();
        for (const char* ptr = data_.data(); ptr != last; ++ptr)
        {
                if (!*ptr)
                {
                        lines_.push_back(beginning);
                        beginning = ptr + 1;
                }
        }

        ASSERT(lines_.size() == line_count);
}
}
