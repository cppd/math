/*
Copyright (C) 2017, 2018 Topological Manifold

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

/*
 Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein.
 Introduction to Algorithms. Third Edition.
 The MIT Press, 2009.

 Глава 29 Linear Programming.
*/

#pragma once

#include "com/error.h"
#include "com/types.h"

#include <array>

// 29.3 The simplex algorithm.
// Pivoting.
template <size_t N, size_t M, typename T>
void pivot(std::array<T, M>& b, std::array<std::array<T, N>, M>& a, T& v, std::array<T, N>& c, unsigned l, unsigned e) noexcept
{
        static_assert(is_native_floating_point<T>);

        ASSERT(l < M);
        ASSERT(e < N);
        ASSERT(a[l][e] != 0);

        // С отличием от алгоритма в книге в том, что пишется в переменные
        // с теми же номерами без замены индексов l и e, а также используется
        // другая работа со знаками.

        b[l] = -b[l] / a[l][e];
        for (unsigned j = 0; j < N; ++j)
        {
                if (j == e)
                {
                        continue;
                }
                a[l][j] = -a[l][j] / a[l][e];
        }
        a[l][e] = 1 / a[l][e];

        for (unsigned i = 0; i < M; ++i)
        {
                if (i == l)
                {
                        continue;
                }
                b[i] = b[i] + a[i][e] * b[l];
                for (unsigned j = 0; j < N; ++j)
                {
                        if (j == e)
                        {
                                continue;
                        }
                        a[i][j] = a[i][j] + a[i][e] * a[l][j];
                }
                a[i][e] = a[i][e] * a[l][e];
        }

        v = v + c[e] * b[l];
        for (unsigned j = 0; j < N; ++j)
        {
                if (j == e)
                {
                        continue;
                }
                c[j] = c[j] + c[e] * a[l][j];
        }
        c[e] = c[e] * a[l][e];
}
