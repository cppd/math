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

#include <src/com/type/trait.h>

#include <string>
#include <type_traits>
#include <utility>

namespace ns
{
template <std::size_t Rows, std::size_t Columns, typename T>
class Matrix
{
        static_assert(is_floating_point<T>);
        static_assert(Rows >= 1 && Columns >= 1);

        template <std::size_t Column, std::size_t... I>
        static constexpr Vector<Columns, T> make_vector_one_value_impl(
                const T& v,
                std::integer_sequence<std::size_t, I...>)
        {
                static_assert(sizeof...(I) == Columns);
                static_assert(((I >= 0 && I < Columns) && ...));
                static_assert(Column >= 0 && Column < Columns);

                return {(I == Column ? v : 0)...};
        }

        template <std::size_t... I>
        static constexpr std::array<Vector<Columns, T>, Rows> make_one_value_rows_impl(
                const T& v,
                std::integer_sequence<std::size_t, I...>)
        {
                static_assert(sizeof...(I) == Rows);
                static_assert(((I >= 0 && I < Rows) && ...));

                return {make_vector_one_value_impl<I>(v, std::make_integer_sequence<std::size_t, Columns>())...};
        }

        static constexpr std::array<Vector<Columns, T>, Rows> make_diagonal_matrix(const T& v)
        {
                return make_one_value_rows_impl(v, std::make_integer_sequence<std::size_t, Rows>());
        }

        //

        std::array<Vector<Columns, T>, Rows> rows_;

        // template <std::size_t... I>
        // constexpr Vector<Rows, T> column_impl(std::size_t column, std::integer_sequence<std::size_t, I...>) const
        //{
        //        static_assert(sizeof...(I) == Rows);
        //        static_assert(((I >= 0 && I < Rows) && ...));
        //
        //        return Vector<Rows, T>{rows_[I][column]...};
        //}
        //
        // constexpr Vector<Rows, T> column_impl(std::size_t column) const
        //{
        //        return column_impl(column, std::make_integer_sequence<std::size_t, Rows>());
        //}

public:
        constexpr Matrix()
        {
        }

        template <typename... Args>
        explicit constexpr Matrix(Args&&... args) requires(
                (sizeof...(args) == Rows) && (std::is_convertible_v<Args, Vector<Columns, T>> && ...))
                : rows_{std::forward<Args>(args)...}
        {
        }

        template <typename Arg>
        explicit constexpr Matrix(const Arg& v) requires(Columns == Rows && std::is_convertible_v<Arg, T>)
                : rows_(make_diagonal_matrix(v))
        {
        }

        explicit constexpr Matrix(const std::array<Vector<Columns, T>, Rows>& data) : rows_(data)
        {
        }

        [[nodiscard]] constexpr const Vector<Columns, T>& row(std::size_t r) const
        {
                return rows_[r];
        }

        [[nodiscard]] constexpr Vector<Columns, T>& row(std::size_t r)
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
                static_assert(sizeof(Matrix) == Rows * Columns * sizeof(T));
                return rows_[0].data();
        }

        [[nodiscard]] Matrix<Columns, Rows, T> transpose() const
        {
                Matrix<Columns, Rows, T> res;
                for (std::size_t r = 0; r < Rows; ++r)
                {
                        for (std::size_t c = 0; c < Columns; ++c)
                        {
                                res(c, r) = rows_[r][c];
                        }
                }
                return res;
        }

        template <std::size_t R = Rows, std::size_t C = Columns>
        [[nodiscard]] T determinant() const requires(R == Rows && C == Columns && Rows == Columns)
        {
                return numerical::determinant(rows_);
        }

        template <std::size_t R = Rows, std::size_t C = Columns>
        [[nodiscard]] Matrix<Rows, Rows, T> inverse() const requires(R == Rows && C == Columns && Rows == Columns)
        {
                return Matrix<Rows, Rows, T>(numerical::inverse(rows_));
        }

        template <std::size_t R = Rows, std::size_t C = Columns>
        [[nodiscard]] Vector<Rows, T> solve(const Vector<Rows, T>& b) const
                requires(R == Rows && C == Columns && Rows == Columns)
        {
                return numerical::linear_solve<Rows, T>(rows_, b);
        }

        template <std::size_t R, std::size_t C>
        [[nodiscard]] Matrix<R, C, T> top_left() const
        {
                static_assert(R > 0 && C > 0 && R <= Rows && C <= Columns && (R < Rows || C < Columns));

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
                static constexpr std::size_t N = std::min(Rows, Columns);

                T s = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        s += rows_[i][i];
                }
                return s;
        }

        [[nodiscard]] Vector<std::min(Rows, Columns), T> diagonal() const
        {
                static constexpr std::size_t N = std::min(Rows, Columns);

                Vector<N, T> d;
                for (std::size_t i = 0; i < N; ++i)
                {
                        d[i] = rows_[i][i];
                }
                return d;
        }
};

template <std::size_t Rows, std::size_t Inner, std::size_t Columns, typename T>
[[nodiscard]] constexpr Matrix<Rows, Columns, T> operator*(
        const Matrix<Rows, Inner, T>& m1,
        const Matrix<Inner, Columns, T>& m2)
{
        Matrix<Rows, Columns, T> res;
        for (std::size_t r = 0; r < Rows; ++r)
        {
                Vector<Columns, T>& row = res.row(r);
                for (std::size_t c = 0; c < Columns; ++c)
                {
                        row[c] = m1(r, 0) * m2(0, c);
                }
                for (std::size_t i = 1; i < Inner; ++i)
                {
                        for (std::size_t c = 0; c < Columns; ++c)
                        {
                                row[c] += m1(r, i) * m2(i, c);
                        }
                }
        }
        return res;
}

template <std::size_t Rows, std::size_t Columns, typename T>
[[nodiscard]] constexpr Vector<Columns, T> operator*(const Vector<Rows, T>& v, const Matrix<Rows, Columns, T>& m)
{
        Vector<Columns, T> res;
        for (std::size_t c = 0; c < Columns; ++c)
        {
                res[c] = v[0] * m(0, c);
        }
        for (std::size_t r = 1; r < Rows; ++r)
        {
                for (std::size_t c = 0; c < Columns; ++c)
                {
                        res[c] += v[r] * m(r, c);
                }
        }
        return res;
}

template <std::size_t Rows, std::size_t Columns, typename T>
[[nodiscard]] constexpr Vector<Rows, T> operator*(const Matrix<Rows, Columns, T>& m, const Vector<Columns, T>& v)
{
        Vector<Rows, T> res;
        for (std::size_t r = 0; r < Rows; ++r)
        {
                res[r] = m(r, 0) * v[0];
                for (std::size_t c = 1; c < Columns; ++c)
                {
                        res[r] += m(r, c) * v[c];
                }
        }
        return res;
}

template <typename Dst, std::size_t Rows, std::size_t Columns, typename Src>
[[nodiscard]] Matrix<Rows, Columns, Dst> to_matrix(const Matrix<Rows, Columns, Src>& m) requires(
        !std::is_same_v<Dst, Src>)
{
        Matrix<Rows, Columns, Dst> res;
        for (std::size_t r = 0; r < Rows; ++r)
        {
                for (std::size_t c = 0; c < Columns; ++c)
                {
                        res(r, c) = m(r, c);
                }
        }
        return res;
}

template <typename Dst, std::size_t Rows, std::size_t Columns, typename Src>
[[nodiscard]] const Matrix<Rows, Columns, Src>& to_matrix(const Matrix<Rows, Columns, Src>& m) requires(
        std::is_same_v<Dst, Src>)
{
        return m;
}

template <std::size_t Rows, std::size_t Columns, typename T>
[[nodiscard]] std::string to_string(const Matrix<Rows, Columns, T>& m)
{
        std::string s;
        s += to_string(m.row(0));
        for (std::size_t r = 1; r < Rows; ++r)
        {
                s += '\n';
                s += to_string(m.row(r));
        }
        return s;
}

using mat3d = Matrix<3, 3, double>;
using mat3f = Matrix<3, 3, float>;
using mat4d = Matrix<4, 4, double>;
using mat4f = Matrix<4, 4, float>;
}
