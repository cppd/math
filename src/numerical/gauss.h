/*
Copyright (C) 2017-2026 Topological Manifold

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

/*
Replace Ax = b by LUx = b -> Ly = b, where Ux = y
1) solve for y: A = LU, Ly = b
2) solve for x: Ux = y
*/

#pragma once

#include "vector.h"

#include <src/com/error.h>
#include <src/com/math.h>
#include <src/com/type/concept.h>

#include <array>
#include <cstddef>

namespace ns::numerical
{
namespace gauss_implementation
{
template <std::size_t N, typename T, template <std::size_t, std::size_t, typename> typename Matrix>
constexpr int find_pivot(const Matrix<N, N, T>& a, const int column, const int from_row)
{
        T max = absolute(a[from_row, column]);
        int pivot = from_row;
        for (int r = from_row + 1, max_r = N; r < max_r; ++r)
        {
                const T v = absolute(a[r, column]);
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

        [[nodiscard]] constexpr const T& operator[](const int r, const int c) const&
        {
                return (*rows_[r])[c];
        }

        [[nodiscard]] constexpr T& operator[](const int r, const int c) &
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

template <std::size_t UN, typename T>
constexpr bool solve_u(RowMatrix<UN, UN, T>& m)
{
        constexpr int N = UN;

        bool sign = false;

        for (int k = 0; k < N - 1; ++k)
        {
                const int pivot = find_pivot(m, k, k);
                if (pivot != k)
                {
                        m.swap(pivot, k);
                        sign = !sign;
                }

                for (int i = k + 1; i < N; ++i)
                {
                        const T l_ik = m[i, k] / m[k, k];
                        for (int j = k + 1; j < N; ++j)
                        {
                                m[i, j] -= l_ik * m[k, j];
                        }
                }
        }

        return sign;
}

template <std::size_t UN, typename T>
constexpr void solve_u_and_y(RowMatrix<UN, UN, T>& a, Vector<UN, T>& b)
{
        constexpr int N = UN;

        for (int k = 0; k < N - 1; ++k)
        {
                const int pivot = find_pivot(a, k, k);
                if (pivot != k)
                {
                        a.swap(pivot, k);
                        std::swap(b[pivot], b[k]);
                }

                for (int i = k + 1; i < N; ++i)
                {
                        const T l_ik = a[i, k] / a[k, k];
                        for (int j = k; j < N; ++j)
                        {
                                a[i, j] -= l_ik * a[k, j];
                        }
                        b[i] -= l_ik * b[k];
                }
        }
}

template <std::size_t UN, std::size_t UM, typename T>
constexpr void solve_u_and_y(RowMatrix<UN, UN, T>& a, RowMatrix<UN, UM, T>& b)
{
        constexpr int N = UN;
        constexpr int M = UM;

        for (int k = 0; k < N - 1; ++k)
        {
                const int pivot = find_pivot(a, k, k);
                if (pivot != k)
                {
                        a.swap(pivot, k);
                        b.swap(pivot, k);
                }

                for (int i = k + 1; i < N; ++i)
                {
                        const T l_ik = a[i, k] / a[k, k];
                        for (int j = k; j < N; ++j)
                        {
                                a[i, j] -= l_ik * a[k, j];
                        }
                        for (int m = 0; m < M; ++m)
                        {
                                b[i, m] -= l_ik * b[k, m];
                        }
                }
        }
}

template <std::size_t UN, typename T>
constexpr void solve_x(RowMatrix<UN, UN, T>& u, Vector<UN, T>& y)
{
        constexpr int N = UN;

        y[N - 1] = y[N - 1] / u[N - 1, N - 1];
        for (int k = N - 2; k >= 0; --k)
        {
                for (int j = k + 1; j < N; ++j)
                {
                        y[k] -= u[k, j] * y[j];
                }
                y[k] = y[k] / u[k, k];
        }
}

template <std::size_t UN, std::size_t UM, typename T>
constexpr void solve_x(RowMatrix<UN, UN, T>& u, RowMatrix<UN, UM, T>& y)
{
        constexpr int N = UN;
        constexpr int M = UM;

        for (int m = 0; m < M; ++m)
        {
                y[N - 1, m] = y[N - 1, m] / u[N - 1, N - 1];
        }
        for (int k = N - 2; k >= 0; --k)
        {
                for (int j = k + 1; j < N; ++j)
                {
                        for (int m = 0; m < M; ++m)
                        {
                                y[k, m] -= u[k, j] * y[j, m];
                        }
                }
                for (int m = 0; m < M; ++m)
                {
                        y[k, m] = y[k, m] / u[k, k];
                }
        }
}

template <std::size_t N, typename T>
constexpr T determinant(RowMatrix<N, N, T>& m)
{
        static_assert(FloatingPoint<T>);
        static_assert(N >= 1);

        const bool sign = solve_u(m);

        T d = m[0, 0];
        for (std::size_t i = 1; i < N; ++i)
        {
                d *= m[i, i];
        }

        return sign ? -d : d;
}

template <std::size_t N, typename T>
constexpr Vector<N, T> solve_gauss(RowMatrix<N, N, T>& a, Vector<N, T>& b)
{
        static_assert(FloatingPoint<T>);
        static_assert(N >= 1);

        solve_u_and_y(a, b);

        solve_x(a, b);

        return b;
}

template <std::size_t N, std::size_t M, typename T>
constexpr std::array<Vector<M, T>, N> solve_gauss(RowMatrix<N, N, T>& a, RowMatrix<N, M, T>& b)
{
        static_assert(FloatingPoint<T>);
        static_assert(N >= 1 && M >= 1);

        solve_u_and_y(a, b);

        solve_x(a, b);

        std::array<Vector<M, T>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::move(b.row(i));
        }
        return res;
}
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr T determinant_gauss(std::array<Vector<N, T>, N> rows)
{
        namespace impl = gauss_implementation;

        impl::RowMatrix<N, N, T> matrix(&rows);
        return impl::determinant(matrix);
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr T determinant_gauss(
        const std::array<Vector<N + 1, T>, N>& rows,
        const std::size_t excluded_column)
{
        namespace impl = gauss_implementation;

        ASSERT(excluded_column <= N);

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
        impl::RowMatrix<N, N, T> matrix(&rows_copy);
        return impl::determinant(matrix);
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> solve_gauss(std::array<Vector<N, T>, N> a, Vector<N, T> b)
{
        namespace impl = gauss_implementation;

        impl::RowMatrix<N, N, T> a_matrix(&a);
        return impl::solve_gauss(a_matrix, b);
}

template <std::size_t N, std::size_t M, typename T>
[[nodiscard]] constexpr std::array<Vector<M, T>, N> solve_gauss(
        std::array<Vector<N, T>, N> a,
        std::array<Vector<M, T>, N> b)
{
        namespace impl = gauss_implementation;

        impl::RowMatrix<N, N, T> a_matrix(&a);
        impl::RowMatrix<N, M, T> b_matrix(&b);
        return impl::solve_gauss(a_matrix, b_matrix);
}
}
