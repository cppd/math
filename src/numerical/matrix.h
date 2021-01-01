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

#include "gauss.h"
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
        static_assert(Rows > 1 && Columns > 1);

        std::array<Vector<Columns, T>, Rows> m_data;

        //

        // template <std::size_t... I>
        // constexpr Vector<Rows, T> column_impl(std::size_t column, std::integer_sequence<std::size_t, I...>) const
        //{
        //        static_assert(sizeof...(I) == Rows);
        //        static_assert(((I >= 0 && I < Rows) && ...));
        //
        //        return Vector<Rows, T>{m_data[I][column]...};
        //}
        //
        // constexpr Vector<Rows, T> column_impl(std::size_t column) const
        //{
        //        return column_impl(column, std::make_integer_sequence<std::size_t, Rows>());
        //}

        //

        template <std::size_t Column, std::size_t... I>
        constexpr Vector<Columns, T> make_vector_one_value_impl(const T& v, std::integer_sequence<std::size_t, I...>)
                const
        {
                static_assert(sizeof...(I) == Columns);
                static_assert(((I >= 0 && I < Columns) && ...));
                static_assert(Column >= 0 && Column < Columns);

                return {(I == Column ? v : 0)...};
        }

        template <std::size_t... I>
        constexpr std::array<Vector<Columns, T>, Rows> make_one_value_rows_impl(
                const T& v,
                std::integer_sequence<std::size_t, I...>) const
        {
                static_assert(sizeof...(I) == Rows);
                static_assert(((I >= 0 && I < Rows) && ...));

                return {make_vector_one_value_impl<I>(v, std::make_integer_sequence<std::size_t, Columns>())...};
        }

        constexpr std::array<Vector<Columns, T>, Rows> make_diagonal_matrix(const T& v) const
        {
                return make_one_value_rows_impl(v, std::make_integer_sequence<std::size_t, Rows>());
        }

public:
        Matrix() = default;

        template <typename... Args>
        explicit constexpr Matrix(Args... args) : m_data{args...}
        {
                static_assert((std::is_same_v<Args, Vector<Columns, T>> && ...));
                static_assert(sizeof...(args) == Rows);
        }

        template <typename Arg>
        explicit constexpr Matrix(const Arg& v) : m_data(make_diagonal_matrix(v))
        {
                static_assert(Columns == Rows);
        }

        constexpr const Vector<Columns, T>& row(std::size_t r) const
        {
                return m_data[r];
        }

        constexpr Vector<Columns, T>& row(std::size_t r)
        {
                return m_data[r];
        }

        constexpr const T& operator()(std::size_t r, std::size_t c) const
        {
                return m_data[r][c];
        }

        constexpr T& operator()(std::size_t r, std::size_t c)
        {
                return m_data[r][c];
        }

        const T* data() const
        {
                static_assert(sizeof(Matrix) == Rows * Columns * sizeof(T));
                return m_data[0].data();
        }

        Matrix<Columns, Rows, T> transpose() const
        {
                Matrix<Columns, Rows, T> res;
                for (std::size_t r = 0; r < Rows; ++r)
                {
                        for (std::size_t c = 0; c < Columns; ++c)
                        {
                                res(c, r) = m_data[r][c];
                        }
                }
                return res;
        }

        template <std::size_t R = Rows, std::size_t C = Columns>
        std::enable_if_t<R == C, T> determinant() const
        {
                static_assert(R == Rows && C == Columns && Rows == Columns);

                Matrix<Rows, Rows, T> m(*this);

                return numerical::determinant_gauss(&m);
        }

        template <std::size_t R = Rows, std::size_t C = Columns>
        std::enable_if_t<R == C, Matrix<Rows, Rows, T>> inverse() const
        {
                static_assert(R == Rows && C == Columns && Rows == Columns);

                Matrix<Rows, Rows, T> m(*this);
                Matrix<Rows, Rows, T> b(1);

                numerical::solve_gauss<Rows, Rows, T>(&m, &b);

                return b;
        }

        template <std::size_t R = Rows, std::size_t C = Columns>
        std::enable_if_t<R == C, Vector<Rows, T>> solve(const Vector<Rows, T>& b) const
        {
                static_assert(R == Rows && C == Columns && Rows == Columns);

                Matrix<Rows, Rows, T> a(*this);
                Vector<Rows, T> x(b);

                numerical::solve_gauss<Rows, T>(&a, &x);

                return x;
        }

        template <std::size_t R, std::size_t C>
        Matrix<R, C, T> top_left() const
        {
                static_assert(R <= Rows && C <= Columns && (R < Rows || C < Columns));
                Matrix<R, C, T> res;
                for (std::size_t r = 0; r < R; ++r)
                {
                        for (std::size_t c = 0; c < C; ++c)
                        {
                                res(r, c) = m_data[r][c];
                        }
                }
                return res;
        }

        T trace() const
        {
                constexpr std::size_t N = std::min(Rows, Columns);
                T s = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        s += m_data[i][i];
                }
                return s;
        }

        Vector<std::min(Rows, Columns), T> diagonal() const
        {
                constexpr std::size_t N = std::min(Rows, Columns);
                Vector<N, T> d;
                for (std::size_t i = 0; i < N; ++i)
                {
                        d[i] = m_data[i][i];
                }
                return d;
        }
};

template <std::size_t Rows, std::size_t Inner, std::size_t Columns, typename T>
Matrix<Rows, Columns, T> operator*(const Matrix<Rows, Inner, T>& m1, const Matrix<Inner, Columns, T>& m2)
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
                                row[c] = std::fma(m1(r, i), m2(i, c), row[c]);
                        }
                }
        }
        return res;
}

template <std::size_t Rows, std::size_t Columns, typename T>
Vector<Columns, T> operator*(const Vector<Rows, T>& v, const Matrix<Rows, Columns, T>& m)
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
                        res[c] = std::fma(v[r], m(r, c), res[c]);
                }
        }
        return res;
}

template <std::size_t Rows, std::size_t Columns, typename T>
Vector<Rows, T> operator*(const Matrix<Rows, Columns, T>& m, const Vector<Columns, T>& v)
{
        Vector<Rows, T> res;
        for (std::size_t r = 0; r < Rows; ++r)
        {
                res[r] = m(r, 0) * v[0];
                for (std::size_t c = 1; c < Columns; ++c)
                {
                        res[r] = std::fma(m(r, c), v[c], res[r]);
                }
        }
        return res;
}

template <typename Dst, std::size_t Rows, std::size_t Columns, typename Src>
std::enable_if_t<!std::is_same_v<Dst, Src>, Matrix<Rows, Columns, Dst>> to_matrix(const Matrix<Rows, Columns, Src>& m)
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
std::enable_if_t<std::is_same_v<Dst, Src>, const Matrix<Rows, Columns, Src>&> to_matrix(
        const Matrix<Rows, Columns, Src>& m)
{
        return m;
}

template <std::size_t Rows, std::size_t Columns, typename T>
std::string to_string(const Matrix<Rows, Columns, T>& m)
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

// Не менять эти типы.
using mat3 = Matrix<3, 3, double>;
using mat3f = Matrix<3, 3, float>;
using mat4 = Matrix<4, 4, double>;
using mat4f = Matrix<4, 4, float>;
}
