/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "error.h"
#include "print.h"

template <typename T>
constexpr T log_2(T n)
{
        if (n <= 0)
        {
                error("Arg for log2 must be positive. Arg = " + to_string(n));
        }

        T b = 0;
        T rem = n;
        while (rem >>= 1)
        {
                ++b;
        }
        return b;
}

template <typename T>
constexpr T log_4(T n)
{
        if (n <= 0)
        {
                error("Arg for log4 must be positive. Arg = " + to_string(n));
        }

        T b = 0;
        T rem = n;
        while (rem >>= 2)
        {
                ++b;
        }
        return b;
}

template <typename T>
constexpr T binary_size(T n)
{
        T b = log_2(n);

        if ((static_cast<T>(1) << b) != n)
        {
                error("Binary size error. " + to_string(n) + " is not a power of 2.");
        }
        return b;
}
