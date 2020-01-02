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

#include "com/bits.h"

// Максимальное значение определителя с размещением последней координаты на параболоиде по значениям других координат
template <size_t N, size_t BITS>
constexpr int max_paraboloid_determinant()
{
        // Например, при N = 4
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // max = x * x * x * (x*x + x*x + x*x) * 4!
        // max = (x ^ (N + 1)) * (N - 1) * N!

        static_assert(N >= 2 && N <= 33);
        static_assert(BITS > 0);

        unsigned __int128 f = 1;
        for (unsigned i = 2; i <= N; ++i)
        {
                f *= i;
        }

        f *= (N - 1);

        return BITS * (N + 1) + log_2(f) + 1;
}

// Максимальное значение исходных данных, размещаемых на параболоиде
template <size_t N, size_t BITS>
constexpr int max_paraboloid_source()
{
        // Например, при N = 4
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // max = x*x + x*x + x*x
        // max = (x ^ 2) * (N - 1)

        return BITS * 2 + log_2(N - 1) + 1;
}

// Максимальное значение определителя
template <size_t N, size_t BITS>
constexpr int max_determinant()
{
        // Например, при N = 4
        // |x x x x|
        // |x x x x|
        // |x x x x|
        // |x x x x|
        // max = x * x * x * x * 4!
        // max = (x ^ N) * N!

        static_assert(N >= 2 && N <= 34);
        static_assert(BITS > 0);

        unsigned __int128 f = 1;
        for (unsigned i = 2; i <= N; ++i)
        {
                f *= i;
        }

        return BITS * N + log_2(f) + 1;
}
