/*
Copyright (C) 2017 Topological Manifold

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
На основе книги

Parallel Scientific Computing in C++ and MPI.
A seamless approach to parallel algorithms and their implementation.
George Em Karniadakis, Robert M. Kirby II.
Cambridge University Press.
*/

#pragma once
#ifndef GAUSS_H
#define GAUSS_H

#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/types.h"

#include <array>

namespace GaussImplementation
{
template <size_t N, typename T>
int find_pivot(const std::array<std::array<T, N>, N>& A, int column, int from_row)
{
        T max = fabs(A[from_row][column]);
        int pivot = from_row;
        for (int r = from_row + 1; r < int(N); ++r)
        {
                T v = fabs(A[r][column]);
                if (v > max)
                {
                        max = v;
                        pivot = r;
                }
        }
        return pivot;
}
}

// input: A * x = b.
// output: b = x; A = upper triangular.
template <size_t Size, typename T>
std::enable_if_t<any_floating_point<T>> linear_solve(std::array<std::array<T, Size>, Size>* A_p, std::array<T, Size>* b_p)
{
        constexpr int N = Size;

        std::array<std::array<T, N>, N>& A = *A_p;
        std::array<T, N>& b = *b_p;

        for (int k = 0; k < N - 1; ++k)
        {
                int pivot = GaussImplementation::find_pivot(A, k, k);
                if (pivot != k)
                {
                        std::swap(A[pivot], A[k]);
                        std::swap(b[pivot], b[k]);
                }

                for (int i = k + 1; i < N; ++i)
                {
                        T l_ik = A[i][k] / A[k][k];
                        for (int j = k; j < N; ++j)
                        {
                                // A[i][j] = A[i][j] - l_ik * A[k][j];
                                A[i][j] = fma(-l_ik, A[k][j], A[i][j]);
                        }
                        // b[i] = b[i] - l_ik * b[k];
                        b[i] = fma(-l_ik, b[k], b[i]);
                }
        }

        b[N - 1] = b[N - 1] / A[N - 1][N - 1];
        for (int k = N - 2; k >= 0; --k)
        {
                for (int j = k + 1; j < N; ++j)
                {
                        // b[k] = b[k] - A[k][j] * b[j];
                        b[k] = fma(-A[k][j], b[j], b[k]);
                }
                b[k] = b[k] / A[k][k];
        }
}

template <size_t Size, typename T>
std::enable_if_t<any_floating_point<T>, T> determinant_gauss(const std::array<std::array<T, Size>, Size>& A_p)
{
        constexpr int N = Size;

        std::array<std::array<T, N>, N> A(A_p);

        bool sign = false;

        for (int k = 0; k < N - 1; ++k)
        {
                int pivot = GaussImplementation::find_pivot(A, k, k);
                if (pivot != k)
                {
                        std::swap(A[pivot], A[k]);
                        sign = !sign;
                }

                for (int i = k + 1; i < N; ++i)
                {
                        T l_ik = A[i][k] / A[k][k];
                        for (int j = k; j < N; ++j)
                        {
                                // A[i][j] = A[i][j] - l_ik * A[k][j];
                                A[i][j] = fma(-l_ik, A[k][j], A[i][j]);
                        }
                }
        }

        T d = 1;
        for (int i = 0; i < N; ++i)
        {
                d *= A[i][i];
        }

        return sign ? -d : d;
}

#endif
