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

#pragma once

#include "com/vec.h"

#include <string>
#include <type_traits>
#include <utility>

template <size_t Rows, size_t Columns, typename T>
class Matrix
{
        static_assert(Rows > 1 && Columns > 1);

        std::array<Vector<Columns, T>, Rows> m_data;

        //

        template <size_t... I>
        constexpr Vector<Rows, T> get_column_impl(int column, std::integer_sequence<size_t, I...>) const
        {
                static_assert(sizeof...(I) == Rows);
                static_assert(((I >= 0 && I < Rows) && ...));

                return Vector<Rows, T>{m_data[I][column]...};
        }

        constexpr Vector<Rows, T> get_column(int column) const
        {
                return get_column_impl(column, std::make_integer_sequence<size_t, Rows>());
        }

        //

        template <size_t Column, size_t... I>
        constexpr Vector<Columns, T> make_vector_one_value_impl(const T& v, std::integer_sequence<size_t, I...>) const
        {
                static_assert(sizeof...(I) == Columns);
                static_assert(((I >= 0 && I < Columns) && ...));
                static_assert(Column >= 0 && Column < Columns);

                return {(I == Column ? v : 0)...};
        }

        template <size_t... I>
        constexpr std::array<Vector<Columns, T>, Rows> make_one_value_rows_impl(const T& v,
                                                                                std::integer_sequence<size_t, I...>) const
        {
                static_assert(sizeof...(I) == Rows);
                static_assert(((I >= 0 && I < Rows) && ...));

                return {{make_vector_one_value_impl<I>(v, std::make_integer_sequence<size_t, Columns>())...}};
        }

        constexpr std::array<Vector<Columns, T>, Rows> make_diagonal_matrix(const T& v) const
        {
                return make_one_value_rows_impl(v, std::make_integer_sequence<size_t, Rows>());
        }

public:
        Matrix() = default;

        template <typename... Args>
        constexpr Matrix(Args... args) : m_data{{args...}}
        {
                static_assert((std::is_same_v<Args, Vector<Columns, T>> && ...));
                static_assert(sizeof...(args) == Rows);
        }

        template <typename Arg>
        explicit constexpr Matrix(const Arg& v) : m_data(make_diagonal_matrix(v))
        {
                static_assert(Columns == Rows);
        }

        constexpr const Vector<Columns, T>& row(int r) const
        {
                return m_data[r];
        }

        constexpr Vector<Rows, T> column(int c) const
        {
                return get_column(c);
        }

        constexpr const Vector<Columns, T>& operator[](int r) const
        {
                return m_data[r];
        }

        Vector<Columns, T>& operator[](int r)
        {
                return m_data[r];
        }
};

template <size_t Rows, size_t Inner, size_t Columns, typename T>
Matrix<Rows, Columns, T> operator*(const Matrix<Rows, Inner, T>& m1, const Matrix<Inner, Columns, T>& m2)
{
        Matrix<Rows, Columns, T> res;
        for (size_t row = 0; row < Rows; ++row)
        {
                for (size_t column = 0; column < Columns; ++column)
                {
                        res[row][column] = dot(m1.row(row), m2.column(column));
                }
        }
        return res;
}

template <size_t Rows, size_t Columns, typename T>
Vector<Rows, T> operator*(const Matrix<Rows, Columns, T>& m, const Vector<Columns, T>& v)
{
        Vector<Rows, T> res;
        for (size_t row = 0; row < Rows; ++row)
        {
                res[row] = dot(m.row(row), v);
        }
        return res;
}

template <size_t Rows, size_t Columns, typename T>
std::string to_string(const Matrix<Rows, Columns, T>& m)
{
        std::string s;
        for (unsigned r = 0; r < Rows; ++r)
        {
                s += to_string(m.row(r));
                if (r != Rows - 1)
                {
                        s += '\n';
                }
        }
        return s;
}

// mat4 - это только Matrix<4, 4, double>.
// Не менять.
using mat4 = Matrix<4, 4, double>;
