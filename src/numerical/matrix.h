/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "matrix_object.h" // IWYU pragma: export
#include "vector.h"

#include <src/com/error.h>

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ns::numerical
{
namespace matrix_implementation
{
template <typename T>
struct MatrixTraits;

template <std::size_t R, std::size_t C, typename T>
struct MatrixTraits<Matrix<R, C, T>> final
{
        using Type = T;
        static constexpr std::size_t ROWS = R;
        static constexpr std::size_t COLUMNS = C;
};

template <typename Matrix>
using Type = MatrixTraits<Matrix>::Type;

template <typename Matrix>
inline constexpr std::size_t ROWS = MatrixTraits<Matrix>::ROWS;

template <typename Matrix>
inline constexpr std::size_t COLUMNS = MatrixTraits<Matrix>::COLUMNS;

template <std::size_t N, typename T, std::size_t COLUMN>
[[nodiscard]] constexpr Vector<N, T> make_vector(const T& v)
{
        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                static_assert(((I >= 0 && I < N) && ...));
                return Vector<N, T>{(I == COLUMN ? v : 0)...};
        }(std::make_integer_sequence<std::size_t, N>());
}
}

template <std::size_t R, std::size_t C, typename T>
[[nodiscard]] constexpr bool operator==(const Matrix<R, C, T>& a, const Matrix<R, C, T>& b)
{
        static_assert(std::is_reference_v<decltype(a.row(0))>);

        for (std::size_t r = 0; r < R; ++r)
        {
                const Vector<C, T>& a_row = a.row(r);
                const Vector<C, T>& b_row = b.row(r);
                for (std::size_t c = 0; c < C; ++c)
                {
                        if (!(a_row[c] == b_row[c]))
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> make_diagonal_matrix(const Vector<N, T>& v)
{
        namespace impl = matrix_implementation;
        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                static_assert(((I >= 0 && I < N) && ...));
                return Matrix<N, N, T>({impl::make_vector<N, T, I>(v[I])...});
        }(std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> make_diagonal_matrix(const T& v)
{
        namespace impl = matrix_implementation;
        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                static_assert(((I >= 0 && I < N) && ...));
                return Matrix<N, N, T>({impl::make_vector<N, T, I>(v)...});
        }(std::make_integer_sequence<std::size_t, N>());
}

template <typename... Matrices>
        requires (sizeof...(Matrices) > 1)
[[nodiscard]] constexpr Matrix<
        (matrix_implementation::ROWS<Matrices> + ...),
        (matrix_implementation::COLUMNS<Matrices> + ...),
        std::tuple_element_t<0, std::tuple<matrix_implementation::Type<Matrices>...>>>
        block_diagonal(const Matrices&... matrices)
{
        namespace impl = matrix_implementation;

        using T = std::tuple_element_t<0, std::tuple<impl::Type<Matrices>...>>;

        static_assert(((impl::ROWS<Matrices> > 0) && ...));
        static_assert(((impl::COLUMNS<Matrices> > 0) && ...));
        static_assert(((std::is_same_v<T, impl::Type<Matrices>>) && ...));

        static constexpr std::size_t RESULT_R = (impl::ROWS<Matrices> + ...);
        static constexpr std::size_t RESULT_C = (impl::COLUMNS<Matrices> + ...);

        Matrix<RESULT_R, RESULT_C, T> res;
        for (std::size_t r = 0; r < RESULT_R; ++r)
        {
                for (std::size_t c = 0; c < RESULT_C; ++c)
                {
                        res[r, c] = 0;
                }
        }

        std::size_t base_r = 0;
        std::size_t base_c = 0;

        const auto copy = [&]<std::size_t R, std::size_t C, typename T>(const Matrix<R, C, T>& matrix)
        {
                for (std::size_t r = 0; r < R; ++r)
                {
                        for (std::size_t c = 0; c < C; ++c)
                        {
                                res[base_r + r, base_c + c] = matrix[r, c];
                        }
                }
                base_r += R;
                base_c += C;
        };

        (copy(matrices), ...);

        return res;
}

template <std::size_t COUNT, std::size_t R, std::size_t C, typename T>
[[nodiscard]] constexpr Matrix<R * COUNT, C * COUNT, T> block_diagonal(const Matrix<R, C, T>& matrix)
{
        static_assert(COUNT > 0);

        constexpr std::size_t RESULT_R = R * COUNT;
        constexpr std::size_t RESULT_C = C * COUNT;

        Matrix<RESULT_R, RESULT_C, T> res;
        for (std::size_t r = 0; r < RESULT_R; ++r)
        {
                for (std::size_t c = 0; c < RESULT_C; ++c)
                {
                        res[r, c] = 0;
                }
        }

        for (std::size_t i = 0; i < COUNT; ++i)
        {
                const std::size_t base_r = i * R;
                const std::size_t base_c = i * C;
                for (std::size_t r = 0; r < R; ++r)
                {
                        for (std::size_t c = 0; c < C; ++c)
                        {
                                res[base_r + r, base_c + c] = matrix[r, c];
                        }
                }
        }

        return res;
}

template <std::size_t START, std::size_t STEP, std::size_t N, typename T>
[[nodiscard]] Vector<N / STEP, T> slice(const Vector<N, T>& v)
{
        static_assert(START < STEP);
        static_assert(N % STEP == 0);

        constexpr std::size_t R_SIZE = N / STEP;

        Vector<R_SIZE, T> res;
        for (std::size_t i = 0; i < R_SIZE; ++i)
        {
                res[i] = v[START + STEP * i];
        }
        return res;
}

template <std::size_t START, std::size_t STEP, std::size_t N, typename T>
[[nodiscard]] Matrix<N / STEP, N / STEP, T> slice(const Matrix<N, N, T>& v)
{
        static_assert(START < STEP);
        static_assert(N % STEP == 0);

        constexpr std::size_t R_SIZE = N / STEP;

        Matrix<R_SIZE, R_SIZE, T> res;
        for (std::size_t r = 0; r < R_SIZE; ++r)
        {
                for (std::size_t c = 0; c < R_SIZE; ++c)
                {
                        res[r, c] = v[START + STEP * r, START + STEP * c];
                }
        }
        return res;
}

template <
        std::size_t START_R,
        std::size_t START_C,
        std::size_t BR,
        std::size_t BC,
        std::size_t R,
        std::size_t C,
        typename T>
[[nodiscard]] Matrix<BR, BC, T> block(const Matrix<R, C, T>& m)
{
        static_assert(START_R + BR <= R);
        static_assert(START_C + BC <= C);

        Matrix<BR, BC, T> res;
        for (std::size_t r = START_R, br = 0; br < BR; ++r, ++br)
        {
                for (std::size_t c = START_C, bc = 0; bc < BC; ++c, ++bc)
                {
                        res[br, bc] = m[r, c];
                }
        }
        return res;
}

template <
        std::size_t START_R,
        std::size_t START_C,
        std::size_t R,
        std::size_t C,
        std::size_t BR,
        std::size_t BC,
        typename T>
void set_block(Matrix<R, C, T>& m, const Matrix<BR, BC, T>& block)
{
        static_assert(START_R + BR <= R);
        static_assert(START_C + BC <= C);

        for (std::size_t r = START_R, br = 0; br < BR; ++r, ++br)
        {
                for (std::size_t c = START_C, bc = 0; bc < BC; ++c, ++bc)
                {
                        m[r, c] = block[br, bc];
                }
        }
}

template <std::size_t R, std::size_t C, std::size_t BR, std::size_t BC, typename T>
void set_block(Matrix<R, C, T>& m, const std::size_t start_r, const std::size_t start_c, const Matrix<BR, BC, T>& block)
{
        ASSERT(start_r + BR <= R);
        ASSERT(start_c + BC <= C);

        for (std::size_t r = start_r, br = 0; br < BR; ++r, ++br)
        {
                for (std::size_t c = start_c, bc = 0; bc < BC; ++c, ++bc)
                {
                        m[r, c] = block[br, bc];
                }
        }
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
[[nodiscard]] constexpr Matrix<ROWS, COLUMNS, T> operator+(
        const Matrix<ROWS, COLUMNS, T>& m1,
        const Matrix<ROWS, COLUMNS, T>& m2)
{
        Matrix<ROWS, COLUMNS, T> res;
        for (std::size_t r = 0; r < ROWS; ++r)
        {
                const Vector<COLUMNS, T>& m1_row = m1.row(r);
                const Vector<COLUMNS, T>& m2_row = m2.row(r);
                Vector<COLUMNS, T>& res_row = res.row(r);
                for (std::size_t c = 0; c < COLUMNS; ++c)
                {
                        res_row[c] = m1_row[c] + m2_row[c];
                }
        }
        return res;
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
[[nodiscard]] constexpr Matrix<ROWS, COLUMNS, T> operator-(
        const Matrix<ROWS, COLUMNS, T>& m1,
        const Matrix<ROWS, COLUMNS, T>& m2)
{
        Matrix<ROWS, COLUMNS, T> res;
        for (std::size_t r = 0; r < ROWS; ++r)
        {
                const Vector<COLUMNS, T>& m1_row = m1.row(r);
                const Vector<COLUMNS, T>& m2_row = m2.row(r);
                Vector<COLUMNS, T>& res_row = res.row(r);
                for (std::size_t c = 0; c < COLUMNS; ++c)
                {
                        res_row[c] = m1_row[c] - m2_row[c];
                }
        }
        return res;
}

template <std::size_t ROWS, std::size_t INNER, std::size_t COLUMNS, typename T>
[[nodiscard]] constexpr Matrix<ROWS, COLUMNS, T> operator*(
        const Matrix<ROWS, INNER, T>& m1,
        const Matrix<INNER, COLUMNS, T>& m2)
{
        Matrix<ROWS, COLUMNS, T> res;
        for (std::size_t r = 0; r < ROWS; ++r)
        {
                Vector<COLUMNS, T>& row = res.row(r);
                for (std::size_t c = 0; c < COLUMNS; ++c)
                {
                        row[c] = m1[r, 0] * m2[0, c];
                }
                for (std::size_t i = 1; i < INNER; ++i)
                {
                        for (std::size_t c = 0; c < COLUMNS; ++c)
                        {
                                row[c] += m1[r, i] * m2[i, c];
                        }
                }
        }
        return res;
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
[[nodiscard]] constexpr Vector<COLUMNS, T> operator*(const Vector<ROWS, T>& v, const Matrix<ROWS, COLUMNS, T>& m)
{
        Vector<COLUMNS, T> res;
        for (std::size_t c = 0; c < COLUMNS; ++c)
        {
                res[c] = v[0] * m[0, c];
        }
        for (std::size_t r = 1; r < ROWS; ++r)
        {
                for (std::size_t c = 0; c < COLUMNS; ++c)
                {
                        res[c] += v[r] * m[r, c];
                }
        }
        return res;
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
[[nodiscard]] constexpr Vector<ROWS, T> operator*(const Matrix<ROWS, COLUMNS, T>& m, const Vector<COLUMNS, T>& v)
{
        Vector<ROWS, T> res;
        for (std::size_t r = 0; r < ROWS; ++r)
        {
                res[r] = m[r, 0] * v[0];
                for (std::size_t c = 1; c < COLUMNS; ++c)
                {
                        res[r] += m[r, c] * v[c];
                }
        }
        return res;
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
[[nodiscard]] constexpr Matrix<ROWS, COLUMNS, T> operator*(const Matrix<ROWS, COLUMNS, T>& m, const T& v)
{
        Matrix<ROWS, COLUMNS, T> res;
        for (std::size_t r = 0; r < ROWS; ++r)
        {
                const Vector<COLUMNS, T>& m_row = m.row(r);
                Vector<COLUMNS, T>& res_row = res.row(r);
                for (std::size_t c = 0; c < COLUMNS; ++c)
                {
                        res_row[c] = m_row[c] * v;
                }
        }
        return res;
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
[[nodiscard]] constexpr Matrix<ROWS, COLUMNS, T> operator*(const T& v, const Matrix<ROWS, COLUMNS, T>& m)
{
        return m * v;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> add_diagonal(const Matrix<N, N, T>& m, const T& d)
{
        Matrix<N, N, T> res = m;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i, i] += d;
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> add_diagonal(const T& d, const Matrix<N, N, T>& m)
{
        return add_diagonal(m, d);
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> add_diagonal(const Matrix<N, N, T>& m, const Vector<N, T>& d)
{
        Matrix<N, N, T> res = m;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i, i] += d[i];
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> add_diagonal(const Vector<N, T>& d, const Matrix<N, N, T>& m)
{
        return add_diagonal(m, d);
}

template <std::size_t R, std::size_t C, typename T>
[[nodiscard]] constexpr Matrix<R, C, T> mul_diagonal(const Matrix<R, C, T>& m, const Vector<C, T>& d)
{
        Matrix<R, C, T> res;
        for (std::size_t r = 0; r < R; ++r)
        {
                for (std::size_t c = 0; c < C; ++c)
                {
                        res[r, c] = m[r, c] * d[c];
                }
        }
        return res;
}

template <std::size_t R, std::size_t C, typename T>
[[nodiscard]] constexpr Matrix<R, C, T> mul_diagonal(const Vector<R, T>& d, const Matrix<R, C, T>& m)
{
        Matrix<R, C, T> res;
        for (std::size_t r = 0; r < R; ++r)
        {
                for (std::size_t c = 0; c < C; ++c)
                {
                        res[r, c] = d[r] * m[r, c];
                }
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> add_md(const Matrix<N, N, T>& m, const Vector<N, T>& d)
{
        return add_diagonal(m, d);
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> add_md(const Vector<N, T>& d, const Matrix<N, N, T>& m)
{
        return add_diagonal(d, m);
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N, N, T> add_md(const Matrix<N, N, T>& a, const Matrix<N, N, T>& b)
{
        return a + b;
}

template <typename Dst, std::size_t ROWS, std::size_t COLUMNS, typename Src>
        requires (!std::is_same_v<Dst, Src>)
[[nodiscard]] Matrix<ROWS, COLUMNS, Dst> to_matrix(const Matrix<ROWS, COLUMNS, Src>& m)
{
        Matrix<ROWS, COLUMNS, Dst> res;
        for (std::size_t r = 0; r < ROWS; ++r)
        {
                for (std::size_t c = 0; c < COLUMNS; ++c)
                {
                        res[r, c] = m[r, c];
                }
        }
        return res;
}

template <typename Dst, std::size_t ROWS, std::size_t COLUMNS, typename Src>
        requires (std::is_same_v<Dst, Src>)
[[nodiscard]] decltype(auto) to_matrix(const Matrix<ROWS, COLUMNS, Src>& m)
{
        return m;
}

template <typename Dst, std::size_t ROWS, std::size_t COLUMNS, typename Src>
        requires (std::is_same_v<Dst, Src>)
[[nodiscard]] decltype(auto) to_matrix(Matrix<ROWS, COLUMNS, Src>&& m)
{
        return std::move(m);
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
[[nodiscard]] bool is_finite(const Matrix<ROWS, COLUMNS, T>& m)
{
        for (std::size_t i = 0; i < ROWS; ++i)
        {
                if (is_finite(m.row(i)))
                {
                        continue;
                }
                return false;
        }
        return true;
}

template <std::size_t N, typename T>
inline constexpr Matrix<N, N, T> IDENTITY_MATRIX = make_diagonal_matrix<N, T>(1);

using Matrix3d = Matrix<3, 3, double>;
using Matrix3f = Matrix<3, 3, float>;
using Matrix4d = Matrix<4, 4, double>;
using Matrix4f = Matrix<4, 4, float>;
}
