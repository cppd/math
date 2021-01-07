/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/constant.h>

#include <cmath>

namespace ns::sampling
{
template <typename T>
T erf_inv(T arg)
{
        static_assert(std::is_floating_point_v<T>);

        static constexpr T a = 8 * (PI<long double> - 3) / (3 * PI<long double> * (4 - PI<long double>));
        static constexpr T k = 2 / (PI<long double> * a);
        static constexpr T a_r = 1 / a;

        T v = std::log((1 - arg) * (1 + arg));
        T v1 = k + v / 2;
        T v2 = v * a_r;
        T r = std::sqrt(std::sqrt(v1 * v1 - v2) - v1);
        return std::copysign(r, arg);
}
}
