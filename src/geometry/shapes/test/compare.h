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
#include <src/com/print.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace ns::geometry::shapes::test
{
template <typename T>
[[nodiscard]] constexpr bool compare(const int epsilon_count, const T v1, const T v2)
{
        static_assert(std::is_floating_point_v<T>);

        if (!(std::isfinite(v1) && std::isfinite(v2) && (v1 > 0) && (v2 > 0)))
        {
                return false;
        }

        const T e = epsilon_count * Limits<T>::epsilon();

        return (v2 > (v1 - v1 * e)) && (v2 < (v1 + v1 * e)) && (v1 > (v2 - v2 * e)) && (v1 < (v2 + v2 * e));
}

template <typename T, typename S>
void compare(S&& name, const T v1, const T v2, const T precision)
{
        static_assert(std::is_floating_point_v<T>);

        if (std::isfinite(v1) && std::isfinite(v2)
            && ((v1 == v2) || (std::abs(v1 - v2) / std::max(std::abs(v1), std::abs(v2)) < precision)))
        {
                return;
        }

        error(std::string(std::forward<S>(name)) + ": numbers are not equal " + to_string(v1) + " and "
              + to_string(v2));
}
}
