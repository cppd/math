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

#include "limit.h"

#include <type_traits>

namespace ns
{
template <typename T>
inline constexpr T PREVIOUS_BEFORE_ONE = []
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(Limits<T>::radix() == 2);

        T prev_e = 1;
        T e = 1;
        do
        {
                prev_e = e;
                e /= 2;
        } while (1 - e != 1);
        return 1 - prev_e;
}();
}
