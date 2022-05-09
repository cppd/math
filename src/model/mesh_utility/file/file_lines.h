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

#include <src/com/error.h>

#include <array>
#include <vector>

namespace ns::mesh::file
{
class Lines final
{
        std::vector<char> data_;
        std::vector<long long> beginnings_;

public:
        explicit Lines(std::vector<char>&& text_data);

        std::size_t size() const
        {
                return beginnings_.size();
        }

        std::array<const char*, 2> c_str_view(const std::size_t line) const
        {
                const char* const first = &data_[beginnings_[line]];

                const long long next_line_beginning =
                        (line + 1 < beginnings_.size()) ? beginnings_[line + 1] : data_.size();
                const char* const last = &data_[next_line_beginning - 1];

                return {first, last};
        }

        const char* c_str(const std::size_t line) const
        {
                return &data_[beginnings_[line]];
        }
};
}
