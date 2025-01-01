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

#include <array>

namespace ns::geometry::accelerators
{
class BvhStack final
{
        static constexpr unsigned STACK_SIZE = 64;

        std::array<unsigned, STACK_SIZE> stack_;
        int next_ = 0;

public:
        void push(const unsigned v)
        {
                stack_[next_++] = v;
        }

        [[nodiscard]] unsigned pop()
        {
                return stack_[--next_];
        }

        [[nodiscard]] bool empty() const
        {
                return next_ == 0;
        }
};
}
