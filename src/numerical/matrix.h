/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "determinant.h"
#include "solve.h"
#include "vector.h"

#include <src/com/error.h>
#include <src/com/type/concept.h>

#include <string>
#include <type_traits>
#include <utility>

namespace ns
{
namespace matrix_implementation
{
template <std::size_t N, typename T, std::size_t COLUMN>
constexpr Vector<N, T> make_vector(const T& v)
{
        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...> &&)
        {
                static_assert(sizeof...(I) == N);
                static_assert(((I >= 0 && I < N) && ...));
                return Vector<N, T>{(I == COLUMN ? v : 0)...};
        }
        (std::make_integer_sequence<std::size_t, N>());
}
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
class Matrix final
{
        static_assert(FloatingPoint<T>);
        static_assert(ROWS >= 1 && COLUMNS >= 1);

        std::array<Vector<COLUMNS, T>, ROWS> rows_;

        // constexpr Vector<ROWS, T> column_impl(const std::size_t column) const
        // {
        //         return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...> &&)
        //         {
        //                 static_assert(sizeof...(I) == ROWS);
        //                 static_assert(((I >= 0 && I < ROWS) && ...));
        //
        //                 return Vector<ROWS, T>{rows_[I][column]...};
        //         }
        //         (std::make_integer_sequence<std::size_t, ROWS>());
        // }

public:
        constexpr Matrix()
        {
        }

        // template <typename... Args>
        //         requires ((sizeof...(Args) == ROWS) && (std::is_convertible_v<Args, Vector<COLUMNS, T>> && ...))
        // explicit constexpr Matrix(Args&&... args)
        //         : rows_{std::forward<Args>(args)...}
        // {
        // }

        constexpr Matrix(std::initializer_list<std::initializer_list<T>>&& data)
        {
                ASSERT(data.size() == ROWS);
                std::size_t r = 0;
                for (auto&& row : data)
                {
                        ASSERT(row.size() == COLUMNS);
                        std::size_t c = 0;
                        for (auto&& column : row)
                        {
                                rows_[r][c] = column;
                                ++c;
                        }
                        ASSERT(c == COLUMNS);
                        ++r;
                }
                ASSERT(r == ROWS);
        }

        explicit constexpr Matrix(const std::array<Vector<COLUMNS, T>, ROWS>& data)
                : rows_(data)
        {
        }

        [[nodiscard]] constexpr const Vector<COLUMNS, T>& row(const std::size_t r) const
        {
                return rows_[r];
        }

        [[nodiscard]] constexpr Vector<COLUMNS, T>& row(const std::size_t r)
        {
                return rows_[r];
        }

        [[nodiscard]] constexpr const T& operator()(const std::size_t r, const std::size_t c) const
        {
                return rows_[r][c];
        }

        [[nodiscard]] constexpr T& operator()(const std::size_t r, const std::size_t c)
        {
                return rows_[r][c];
        }

        [[nodiscard]] const T* data() const
        {
                static_assert(sizeof(Matrix) == ROWS * COLUMNS * sizeof(T));
                return rows_[0].data();
        }

        [[nodiscard]] Matrix<COLUMNS, ROWS, T> transpose() const
        {
                Matrix<COLUMNS, ROWS, T> res;
                for (std::size_t r = 0; r < ROWS; ++r)
                {
                        for (std::size_t c = 0; c < COLUMNS; ++c)
                        {
                                res(c, r) = rows_[r][c];
                        }
                }
                return res;
        }

        [[nodiscard]] T determinant() const
                requires (ROWS == COLUMNS)
        {
                return numerical::determinant(rows_);
        }

        [[nodiscard]] Matrix<ROWS, ROWS, T> inverse() const
                requires (ROWS == COLUMNS)
        {
                return Matrix<ROWS, ROWS, T>(numerical::inverse(rows_));
        }

        [[nodiscard]] Vector<ROWS, T> solve(const Vector<ROWS, T>& b) const
                requires (ROWS == COLUMNS)
        {
                return numerical::linear_solve<ROWS, T>(rows_, b);
        }

        template <std::size_t R, std::size_t C>
        [[nodiscard]] Matrix<R, C, T> top_left() const
        {
                static_assert(R > 0 && C > 0 && R <= ROWS && C <= COLUMNS && (R < ROWS || C < COLUMNS));

                Matrix<R, C, T> res;
                for (std::size_t r = 0; r < R; ++r)
                {
                        for (std::size_t c = 0; c < C; ++c)
                        {
                                res(r, c) = rows_[r][c];
                        }
                }
                return res;
        }

        [[nodiscard]] T trace() const
        {
                static constexpr std::size_t N = std::min(ROWS, COLUMNS);

                T res = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res += rows_[i][i];
                }
                return res;
        }

        [[nodiscard]] Vector<std::min(ROWS, COLUMNS), T> diagonal() const
        {
                static constexpr std::size_t N = std::min(ROWS, COLUMNS);

                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = rows_[i][i];
                }
                return res;
        }
};

template <std::size_t N, typename T>
constexpr Matrix<N, N, T> make_diagonal_matrix(const Vector<N, T>& v)
{
        namespace impl = matrix_implementation;
        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...> &&)
        {
                static_assert(sizeof...(I) == N);
                static_assert(((I >= 0 && I < N) && ...));
                return Matrix<N, N, T>({impl::make_vector<N, T, I>(v[I])...});
        }
        (std::make_integer_sequence<std::size_t, N>());
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
                        row[c] = m1(r, 0) * m2(0, c);
                }
                for (std::size_t i = 1; i < INNER; ++i)
                {
                        for (std::size_t c = 0; c < COLUMNS; ++c)
                        {
                                row[c] += m1(r, i) * m2(i, c);
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
                res[c] = v[0] * m(0, c);
        }
        for (std::size_t r = 1; r < ROWS; ++r)
        {
                for (std::size_t c = 0; c < COLUMNS; ++c)
                {
                        res[c] += v[r] * m(r, c);
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
                res[r] = m(r, 0) * v[0];
                for (std::size_t c = 1; c < COLUMNS; ++c)
                {
                        res[r] += m(r, c) * v[c];
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

template <typename Dst, std::size_t ROWS, std::size_t COLUMNS, typename Src>
        requires (!std::is_same_v<Dst, Src>)
[[nodiscard]] Matrix<ROWS, COLUMNS, Dst> to_matrix(const Matrix<ROWS, COLUMNS, Src>& m)
{
        Matrix<ROWS, COLUMNS, Dst> res;
        for (std::size_t r = 0; r < ROWS; ++r)
        {
                for (std::size_t c = 0; c < COLUMNS; ++c)
                {
                        res(r, c) = m(r, c);
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
[[nodiscard]] std::string to_string(const Matrix<ROWS, COLUMNS, T>& m)
{
        std::string s;
        s += to_string(m.row(0));
        for (std::size_t r = 1; r < ROWS; ++r)
        {
                s += '\n';
                s += to_string(m.row(r));
        }
        return s;
}

template <std::size_t N, typename T>
inline constexpr Matrix<N, N, T> IDENTITY_MATRIX = make_diagonal_matrix(Vector<N, T>(1));

using Matrix3d = Matrix<3, 3, double>;
using Matrix3f = Matrix<3, 3, float>;
using Matrix4d = Matrix<4, 4, double>;
using Matrix4f = Matrix<4, 4, float>;
}
