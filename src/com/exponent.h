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

namespace ns
{
template <typename T>
constexpr T square(const T& v)
{
        return v * v;
}

template <unsigned EXPONENT, typename T>
constexpr T power([[maybe_unused]] T base)
{
        if constexpr (EXPONENT == 0)
        {
                return 1;
        }
        if constexpr (EXPONENT == 1)
        {
                return base;
        }
        if constexpr (EXPONENT == 2)
        {
                return base * base;
        }
        if constexpr (EXPONENT == 3)
        {
                return base * base * base;
        }
        if constexpr (EXPONENT == 4)
        {
                T t = base * base;
                return t * t;
        }
        if constexpr (EXPONENT == 5)
        {
                T t = base * base;
                return t * t * base;
        }
        if constexpr (EXPONENT == 6)
        {
                T t = base * base;
                return t * t * t;
        }
        if constexpr (EXPONENT == 7)
        {
                T t = base * base;
                return t * t * t * base;
        }
        if constexpr (EXPONENT == 8)
        {
                T t = base * base;
                T t2 = t * t;
                return t2 * t2;
        }
        if constexpr (EXPONENT >= 9)
        {
                unsigned exp = EXPONENT;

                T res = (exp & 1) ? base : 1;
                exp >>= 1;
                while (exp)
                {
                        base *= base;
                        if (exp & 1)
                        {
                                res *= base;
                        }
                        exp >>= 1;
                }

                return res;
        }
}
}
