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

/*
На основе книги

Parallel Scientific Computing in C++ and MPI.
A seamless approach to parallel algorithms and their implementation.
George Em Karniadakis, Robert M. Kirby II.
Cambridge University Press.
*/

#pragma once

#include <src/com/type/trait.h>

#include <cmath>

namespace ns::numerical
{
namespace gauss_implementation
{
template <std::size_t N, typename T, template <std::size_t, std::size_t, typename> typename Matrix>
int find_pivot(const Matrix<N, N, T>& A, int column, int from_row)
{
        T max = std::abs(A(from_row, column));
        int pivot = from_row;
        for (int r = from_row + 1; r < int(N); ++r)
        {
                T v = std::abs(A(r, column));
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
template <
        std::size_t Size,
        typename T,
        template <std::size_t, std::size_t, typename>
        typename Matrix,
        template <std::size_t, typename>
        typename Vector>
void solve_gauss(Matrix<Size, Size, T>* A_p, Vector<Size, T>* b_p)
{
        static_assert(is_floating_point<T>);

        constexpr int N = Size;

        Matrix<N, N, T>& A = *A_p;
        Vector<N, T>& b = *b_p;

        for (int k = 0; k < N - 1; ++k)
        {
                int pivot = gauss_implementation::find_pivot(A, k, k);
                if (pivot != k)
                {
                        std::swap(A.row(pivot), A.row(k));
                        std::swap(b[pivot], b[k]);
                }

                for (int i = k + 1; i < N; ++i)
                {
                        T l_ik = A(i, k) / A(k, k);
                        for (int j = k; j < N; ++j)
                        {
                                // A(i, j) = A(i, j) - l_ik * A(k, j);
                                A(i, j) = std::fma(-l_ik, A(k, j), A(i, j));
                        }
                        // b[i] = b[i] - l_ik * b[k];
                        b[i] = std::fma(-l_ik, b[k], b[i]);
                }
        }

        b[N - 1] = b[N - 1] / A(N - 1, N - 1);
        for (int k = N - 2; k >= 0; --k)
        {
                for (int j = k + 1; j < N; ++j)
                {
                        // b[k] = b[k] - A(k, j) * b[j];
                        b[k] = std::fma(-A(k, j), b[j], b[k]);
                }
                b[k] = b[k] / A(k, k);
        }
}

// Тоже самое, что и для одного столбца b, только сразу для SizeB столбцов B.
// Если B является единичной матрицей, то в B будет обратная к A матрица.
template <
        std::size_t SizeA,
        std::size_t SizeB,
        typename T,
        template <std::size_t, std::size_t, typename>
        typename Matrix>
void solve_gauss(Matrix<SizeA, SizeA, T>* A_p, Matrix<SizeA, SizeB, T>* B_p)
{
        static_assert(is_floating_point<T>);

        constexpr int N = SizeA;
        constexpr int NB = SizeB;

        Matrix<N, N, T>& A = *A_p;
        Matrix<N, NB, T>& B = *B_p;

        for (int k = 0; k < N - 1; ++k)
        {
                int pivot = gauss_implementation::find_pivot(A, k, k);
                if (pivot != k)
                {
                        std::swap(A.row(pivot), A.row(k));
                        std::swap(B.row(pivot), B.row(k));
                }

                for (int i = k + 1; i < N; ++i)
                {
                        T l_ik = A(i, k) / A(k, k);
                        for (int j = k; j < N; ++j)
                        {
                                // A(i, j) = A(i, j) - l_ik * A(k, j);
                                A(i, j) = std::fma(-l_ik, A(k, j), A(i, j));
                        }
                        for (int n = 0; n < NB; ++n)
                        {
                                // B(i, n) = B(i, n) - l_ik * B(k, n);
                                B(i, n) = std::fma(-l_ik, B(k, n), B(i, n));
                        }
                }
        }

        for (int n = 0; n < NB; ++n)
        {
                B(N - 1, n) = B(N - 1, n) / A(N - 1, N - 1);
        }

        for (int k = N - 2; k >= 0; --k)
        {
                for (int j = k + 1; j < N; ++j)
                {
                        for (int n = 0; n < NB; ++n)
                        {
                                // B(k, n) = B(k, n) - A(k, j) * B(j, n);
                                B(k, n) = std::fma(-A(k, j), B(j, n), B(k, n));
                        }
                }
                for (int n = 0; n < NB; ++n)
                {
                        B(k, n) = B(k, n) / A(k, k);
                }
        }
}

template <std::size_t Size, typename T, template <std::size_t, std::size_t, typename> typename Matrix>
T determinant_gauss(Matrix<Size, Size, T>* A_p)
{
        static_assert(is_floating_point<T>);

        constexpr int N = Size;

        Matrix<N, N, T>& A = *A_p;

        bool sign = false;

        for (int k = 0; k < N - 1; ++k)
        {
                int pivot = gauss_implementation::find_pivot(A, k, k);
                if (pivot != k)
                {
                        std::swap(A.row(pivot), A.row(k));
                        sign = !sign;
                }

                for (int i = k + 1; i < N; ++i)
                {
                        T l_ik = A(i, k) / A(k, k);
                        for (int j = k; j < N; ++j)
                        {
                                // A(i, j) = A(i, j) - l_ik * A(k, j);
                                A(i, j) = std::fma(-l_ik, A(k, j), A(i, j));
                        }
                }
        }

        T d = 1;
        for (int i = 0; i < N; ++i)
        {
                d *= A(i, i);
        }

        return sign ? -d : d;
}
}
