/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <array>
#include <cstddef>
#include <vector>

namespace ns::model::mesh::file
{
class Lines final
{
        std::vector<char> data_;
        std::vector<const char*> lines_;

public:
        explicit Lines(std::vector<char>&& text_data);

        Lines(const Lines&) = delete;
        Lines& operator=(const Lines&) = delete;
        Lines(Lines&&) = delete;
        Lines& operator=(Lines&&) = delete;

        [[nodiscard]] std::size_t size() const
        {
                return lines_.size();
        }

        [[nodiscard]] std::array<const char*, 2> c_str_view(const std::size_t line) const
        {
                return {lines_[line], (line + 1 < lines_.size()) ? lines_[line + 1] - 1 : &data_.back()};
        }

        [[nodiscard]] const char* c_str(const std::size_t line) const
        {
                return lines_[line];
        }
};
}
