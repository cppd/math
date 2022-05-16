/*
Copyright (C) 2017-2022 Topological Manifold

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
Parallel Scientific Computing in C++ and MPI.
A seamless approach to parallel algorithms and their implementation.
George Em Karniadakis, Robert M. Kirby II.
Cambridge University Press.
*/

#pragma once

#include "vector.h"

#include <src/com/math.h>
#include <src/com/type/concept.h>

#include <array>
#include <cmath>

namespace ns::numerical
{
namespace gauss_implementation
{
template <std::size_t N, typename T, template <std::size_t, std::size_t, typename> typename Matrix>
constexpr int find_pivot(const Matrix<N, N, T>& a, const int column, const int from_row)
{
        T max = absolute(a(from_row, column));
        int pivot = from_row;
        for (int r = from_row + 1, max_r = N; r < max_r; ++r)
        {
                T v = absolute(a(r, column));
                if (v > max)
                {
                        max = v;
                        pivot = r;
                }
        }
        return pivot;
}

template <std::size_t R, std::size_t C, typename T>
class RowMatrix final
{
        std::array<Vector<C, T>*, R> rows_;

public:
        explicit constexpr RowMatrix(std::array<Vector<C, T>, R>* const rows)
        {
                for (std::size_t i = 0; i < R; ++i)
                {
                        rows_[i] = rows->data() + i;
                }
        }

        [[nodiscard]] constexpr const T& operator()(const int r, const int c) const&
        {
                return (*rows_[r])[c];
        }

        [[nodiscard]] constexpr T& operator()(const int r, const int c) &
        {
                return (*rows_[r])[c];
        }

        [[nodiscard]] constexpr Vector<C, T>& row(const int r) &
        {
                return *rows_[r];
        }

        constexpr void swap(const int row_1, const int row_2) &
        {
                std::swap(rows_[row_1], rows_[row_2]);
        }
};

template <std::size_t SIZE, typename T>
constexpr T determinant(RowMatrix<SIZE, SIZE, T>&& m)
{
        static_assert(FloatingPoint<T>);
        static_assert(SIZE >= 1);

        constexpr int N = SIZE;

        bool sign = false;

        for (int k = 0; k < N - 1; ++k)
        {
                int pivot = find_pivot(m, k, k);
                if (pivot != k)
                {
                        m.swap(pivot, k);
                        sign = !sign;
                }

                for (int i = k + 1; i < N; ++i)
                {
                        T l_ik = m(i, k) / m(k, k);
                        for (int j = k + 1; j < N; ++j)
                        {
                                m(i, j) -= l_ik * m(k, j);
                        }
                }
        }

        T d = m(0, 0);
        for (int i = 1; i < N; ++i)
        {
                d *= m(i, i);
        }

        return sign ? -d : d;
}

template <std::size_t SIZE, typename T>
constexpr Vector<SIZE, T> solve_gauss(RowMatrix<SIZE, SIZE, T>&& a, Vector<SIZE, T> b)
{
        static_assert(FloatingPoint<T>);

        constexpr int N = SIZE;

        for (int k = 0; k < N - 1; ++k)
        {
                int pivot = find_pivot(a, k, k);
                if (pivot != k)
                {
                        a.swap(pivot, k);
                        std::swap(b[pivot], b[k]);
                }

                for (int i = k + 1; i < N; ++i)
                {
                        T l_ik = a(i, k) / a(k, k);
                        for (int j = k; j < N; ++j)
                        {
                                a(i, j) -= l_ik * a(k, j);
                        }
                        b[i] -= l_ik * b[k];
                }
        }

        b[N - 1] = b[N - 1] / a(N - 1, N - 1);
        for (int k = N - 2; k >= 0; --k)
        {
                for (int j = k + 1; j < N; ++j)
                {
                        b[k] -= a(k, j) * b[j];
                }
                b[k] = b[k] / a(k, k);
        }

        return b;
}

template <std::size_t SIZE_A, std::size_t SIZE_B, typename T>
constexpr std::array<Vector<SIZE_B, T>, SIZE_A> solve_gauss(
        RowMatrix<SIZE_A, SIZE_A, T>&& a,
        RowMatrix<SIZE_A, SIZE_B, T>&& b)
{
        static_assert(FloatingPoint<T>);

        constexpr int N = SIZE_A;
        constexpr int M = SIZE_B;

        for (int k = 0; k < N - 1; ++k)
        {
                int pivot = find_pivot(a, k, k);
                if (pivot != k)
                {
                        a.swap(pivot, k);
                        b.swap(pivot, k);
                }

                for (int i = k + 1; i < N; ++i)
                {
                        T l_ik = a(i, k) / a(k, k);
                        for (int j = k; j < N; ++j)
                        {
                                a(i, j) -= l_ik * a(k, j);
                        }
                        for (int m = 0; m < M; ++m)
                        {
                                b(i, m) -= l_ik * b(k, m);
                        }
                }
        }

        for (int m = 0; m < M; ++m)
        {
                b(N - 1, m) = b(N - 1, m) / a(N - 1, N - 1);
        }

        for (int k = N - 2; k >= 0; --k)
        {
                for (int j = k + 1; j < N; ++j)
                {
                        for (int m = 0; m < M; ++m)
                        {
                                b(k, m) -= a(k, j) * b(j, m);
                        }
                }
                for (int m = 0; m < M; ++m)
                {
                        b(k, m) = b(k, m) / a(k, k);
                }
        }

        std::array<Vector<M, T>, N> res;
        for (int i = 0; i < N; ++i)
        {
                res[i] = std::move(b.row(i));
        }
        return res;
}
}

template <std::size_t N, typename T>
constexpr T determinant_gauss(const std::array<Vector<N, T>, N>& rows)
{
        namespace impl = gauss_implementation;

        std::array<Vector<N, T>, N> rows_copy{rows};
        return impl::determinant(impl::RowMatrix<N, N, T>(&rows_copy));
}

template <std::size_t N, typename T>
constexpr T determinant_gauss(const std::array<Vector<N + 1, T>, N>& rows, const std::size_t excluded_column)
{
        namespace impl = gauss_implementation;

        std::array<Vector<N, T>, N> rows_copy;
        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t c = 0; c < excluded_column; ++c)
                {
                        rows_copy[r][c] = rows[r][c];
                }
                for (std::size_t c = excluded_column; c < N; ++c)
                {
                        rows_copy[r][c] = rows[r][c + 1];
                }
        }
        return impl::determinant(impl::RowMatrix<N, N, T>(&rows_copy));
}

template <std::size_t N, typename T>
constexpr Vector<N, T> solve_gauss(const std::array<Vector<N, T>, N>& a, const Vector<N, T>& b)
{
        namespace impl = gauss_implementation;

        std::array<Vector<N, T>, N> a_copy{a};
        return impl::solve_gauss(impl::RowMatrix<N, N, T>(&a_copy), b);
}

template <std::size_t N, std::size_t M, typename T>
constexpr std::array<Vector<M, T>, N> solve_gauss(
        const std::array<Vector<N, T>, N>& a,
        const std::array<Vector<M, T>, N>& b)
{
        namespace impl = gauss_implementation;

        std::array<Vector<N, T>, N> a_copy{a};
        std::array<Vector<M, T>, N> b_copy{b};
        return impl::solve_gauss(impl::RowMatrix<N, N, T>(&a_copy), impl::RowMatrix<N, M, T>(&b_copy));
}
}
