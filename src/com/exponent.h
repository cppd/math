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

#include <algorithm>
#include <cmath>

namespace ns
{
template <typename T>
[[nodiscard]] constexpr T square(const T& v)
{
        return v * v;
}

template <typename T>
[[nodiscard]] T sqrt_s(const T v)
{
        return std::sqrt(std::max(T{0}, v));
}

template <unsigned EXPONENT, typename T>
[[nodiscard]] constexpr T power(const T& base)
{
        switch (EXPONENT)
        {
        case 0:
        {
                return 1;
        }
        case 1:
        {
                return base;
        }
        case 2:
        {
                return base * base;
        }
        case 3:
        {
                return base * base * base;
        }
        case 4:
        {
                const T t = base * base;
                return t * t;
        }
        case 5:
        {
                const T t = base * base;
                return t * t * base;
        }
        case 6:
        {
                const T t = base * base;
                return t * t * t;
        }
        case 7:
        {
                const T t = base * base;
                return t * t * t * base;
        }
        case 8:
        {
                const T t = base * base;
                const T t2 = t * t;
                return t2 * t2;
        }
        default:
        {
                T res = (EXPONENT & 1) ? base : 1;

                T b = base;
                unsigned exp = EXPONENT >> 1;

                while (exp)
                {
                        b *= b;
                        if (exp & 1)
                        {
                                res *= b;
                        }
                        exp >>= 1;
                }

                return res;
        }
        }
}
}
