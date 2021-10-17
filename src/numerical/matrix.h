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

#include "determinant.h"
#include "solve.h"
#include "vec.h"

#include <src/com/type/concept.h>

#include <string>
#include <type_traits>
#include <utility>

namespace ns
{
template <std::size_t ROWS, std::size_t COLUMNS, typename T>
class Matrix final
{
        static_assert(FloatingPoint<T>);
        static_assert(ROWS >= 1 && COLUMNS >= 1);

        template <std::size_t COLUMN, std::size_t... I>
        static constexpr Vector<COLUMNS, T> make_vector_one_value_impl(
                const T& v,
                std::integer_sequence<std::size_t, I...>)
        {
                static_assert(sizeof...(I) == COLUMNS);
                static_assert(((I >= 0 && I < COLUMNS) && ...));
                static_assert(COLUMN >= 0 && COLUMN < COLUMNS);

                return {(I == COLUMN ? v : 0)...};
        }

        template <std::size_t... I>
        static constexpr std::array<Vector<COLUMNS, T>, ROWS> make_one_value_rows_impl(
                const T& v,
                std::integer_sequence<std::size_t, I...>)
        {
                static_assert(sizeof...(I) == ROWS);
                static_assert(((I >= 0 && I < ROWS) && ...));

                return {make_vector_one_value_impl<I>(v, std::make_integer_sequence<std::size_t, COLUMNS>())...};
        }

        static constexpr std::array<Vector<COLUMNS, T>, ROWS> make_diagonal_matrix(const T& v)
        {
                return make_one_value_rows_impl(v, std::make_integer_sequence<std::size_t, ROWS>());
        }

        //

        std::array<Vector<COLUMNS, T>, ROWS> rows_;

        // template <std::size_t... I>
        // constexpr Vector<ROWS, T> column_impl(std::size_t column, std::integer_sequence<std::size_t, I...>) const
        // {
        //         static_assert(sizeof...(I) == ROWS);
        //         static_assert(((I >= 0 && I < ROWS) && ...));
        //
        //         return Vector<ROWS, T>{rows_[I][column]...};
        // }
        //
        // constexpr Vector<ROWES, T> column_impl(std::size_t column) const
        // {
        //         return column_impl(column, std::make_integer_sequence<std::size_t, ROWS>());
        // }

public:
        constexpr Matrix()
        {
        }

        template <typename... Args>
        explicit constexpr Matrix(Args&&... args) requires(
                (sizeof...(args) == ROWS) && (std::is_convertible_v<Args, Vector<COLUMNS, T>> && ...))
                : rows_{std::forward<Args>(args)...}
        {
        }

        template <typename Arg>
        explicit constexpr Matrix(const Arg& v) requires(COLUMNS == ROWS && std::is_convertible_v<Arg, T>)
                : rows_(make_diagonal_matrix(v))
        {
        }

        explicit constexpr Matrix(const std::array<Vector<COLUMNS, T>, ROWS>& data) : rows_(data)
        {
        }

        [[nodiscard]] constexpr const Vector<COLUMNS, T>& row(std::size_t r) const
        {
                return rows_[r];
        }

        [[nodiscard]] constexpr Vector<COLUMNS, T>& row(std::size_t r)
        {
                return rows_[r];
        }

        [[nodiscard]] constexpr const T& operator()(std::size_t r, std::size_t c) const
        {
                return rows_[r][c];
        }

        [[nodiscard]] constexpr T& operator()(std::size_t r, std::size_t c)
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

        [[nodiscard]] T determinant() const requires(ROWS == COLUMNS)
        {
                return numerical::determinant(rows_);
        }

        [[nodiscard]] Matrix<ROWS, ROWS, T> inverse() const requires(ROWS == COLUMNS)
        {
                return Matrix<ROWS, ROWS, T>(numerical::inverse(rows_));
        }

        [[nodiscard]] Vector<ROWS, T> solve(const Vector<ROWS, T>& b) const requires(ROWS == COLUMNS)
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

                T s = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        s += rows_[i][i];
                }
                return s;
        }

        [[nodiscard]] Vector<std::min(ROWS, COLUMNS), T> diagonal() const
        {
                static constexpr std::size_t N = std::min(ROWS, COLUMNS);

                Vector<N, T> d;
                for (std::size_t i = 0; i < N; ++i)
                {
                        d[i] = rows_[i][i];
                }
                return d;
        }
};

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

template <typename Dst, std::size_t ROWS, std::size_t COLUMNS, typename Src>
[[nodiscard]] Matrix<ROWS, COLUMNS, Dst> to_matrix(const Matrix<ROWS, COLUMNS, Src>& m) requires(
        !std::is_same_v<Dst, Src>)
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
[[nodiscard]] const Matrix<ROWS, COLUMNS, Src>& to_matrix(const Matrix<ROWS, COLUMNS, Src>& m) requires(
        std::is_same_v<Dst, Src>)
{
        return m;
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

using Matrix3d = Matrix<3, 3, double>;
using Matrix3f = Matrix<3, 3, float>;
using Matrix4d = Matrix<4, 4, double>;
using Matrix4f = Matrix<4, 4, float>;
}
