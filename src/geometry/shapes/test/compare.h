/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <src/com/math.h>
#include <src/com/print.h>

#include <algorithm>
#include <cmath>

namespace ns::geometry::shapes::test
{
template <typename T>
constexpr bool compare(const int epsilon_count, const T v1, const T v2)
{
        static_assert(std::is_floating_point_v<T>);

        return is_finite(v1) && is_finite(v2) && (v1 > 0) && (v2 > 0)
               && v2 > (v1 - v1 * (epsilon_count * Limits<T>::epsilon()))
               && v2 < (v1 + v1 * (epsilon_count * Limits<T>::epsilon()))
               && v1 > (v2 - v2 * (epsilon_count * Limits<T>::epsilon()))
               && v1 < (v2 + v2 * (epsilon_count * Limits<T>::epsilon()));
}

static_assert(compare(1, 1.1, 1.1));
static_assert(compare(1000, 10000.100000001, 10000.100000002));
static_assert(!compare(1, 10000.100000001, 10000.100000002));
static_assert(!compare(1, 10000.100000002, 10000.100000001));

template <typename T>
void compare(const char* const name, const T v1, const T v2, const T precision)
{
        if (!(is_finite(v1) && is_finite(v2) && ((v1 == v2) || std::abs((v1 - v2) / std::max(v1, v2)) < precision)))
        {
                error(std::string(name) + ": numbers are not equal " + to_string(v1) + " and " + to_string(v2));
        }
}
}
