/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/type/trait.h"
#include "com/vec.h"

#include <string>
#include <type_traits>
#include <utility>

template <size_t Rows, size_t Columns, typename T>
class Matrix
{
        static_assert(is_floating_point<T>);
        static_assert(Rows > 1 && Columns > 1);

        std::array<Vector<Columns, T>, Rows> m_data;

        //

        template <size_t... I>
        constexpr Vector<Rows, T> column_impl(int column, std::integer_sequence<size_t, I...>) const
        {
                static_assert(sizeof...(I) == Rows);
                static_assert(((I >= 0 && I < Rows) && ...));

                return Vector<Rows, T>{m_data[I][column]...};
        }

        constexpr Vector<Rows, T> column_impl(int column) const
        {
                return column_impl(column, std::make_integer_sequence<size_t, Rows>());
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

                return {make_vector_one_value_impl<I>(v, std::make_integer_sequence<size_t, Columns>())...};
        }

        constexpr std::array<Vector<Columns, T>, Rows> make_diagonal_matrix(const T& v) const
        {
                return make_one_value_rows_impl(v, std::make_integer_sequence<size_t, Rows>());
        }

public:
        Matrix() = default;

        template <typename... Args>
        constexpr Matrix(Args... args) : m_data{args...}
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
                return column_impl(c);
        }

        constexpr const Vector<Columns, T>& operator[](int r) const
        {
                return m_data[r];
        }

        constexpr Vector<Columns, T>& operator[](int r)
        {
                return m_data[r];
        }

        const T* data() const
        {
                static_assert(sizeof(Matrix) == Rows * Columns * sizeof(T));
                return m_data[0].data();
        }
};

template <size_t Rows, size_t Inner, size_t Columns, typename T>
Matrix<Rows, Columns, T> operator*(const Matrix<Rows, Inner, T>& m1, const Matrix<Inner, Columns, T>& m2)
{
        Matrix<Rows, Columns, T> res;
        for (unsigned row = 0; row < Rows; ++row)
        {
                for (unsigned column = 0; column < Columns; ++column)
                {
#if 0
                        // GCC 7.2 имеет проблемы с этим.
                        // Clang 5 не имеет проблем с этим.
                        res[row][column] = dot(m1.row(row), m2.column(column));
#else
                        // GCC 7.2 и Clang 5 с этим проблем не имеют.
                        res[row][column] = m1[row][0] * m2[0][column];
                        for (unsigned i = 1; i < Inner; ++i)
                        {
                                res[row][column] = any_fma(m1[row][i], m2[i][column], res[row][column]);
                        }
#endif
                }
        }
        return res;
}

namespace matrix_implementation
{
template <size_t Rows, size_t Columns, typename T, size_t... I>
Vector<Rows, T> mul_impl(const Matrix<Rows, Columns, T>& m, const Vector<Columns, T>& v, std::integer_sequence<size_t, I...>)
{
        static_assert(sizeof...(I) == Rows);
        static_assert(((I < Rows) && ...));

        return {dot(m.row(I), v)...};
}
}

template <size_t Rows, size_t Columns, typename T>
Vector<Rows, T> operator*(const Matrix<Rows, Columns, T>& m, const Vector<Columns, T>& v)
{
        return matrix_implementation::mul_impl(m, v, std::make_integer_sequence<size_t, Rows>());
}

namespace matrix_implementation
{
template <size_t Rows, size_t Columns, typename T, size_t... I>
constexpr Matrix<Columns, Rows, T> transpose_impl(const Matrix<Rows, Columns, T>& m, std::integer_sequence<size_t, I...>)
{
        static_assert(sizeof...(I) == Columns);
        static_assert(((I < Columns) && ...));

        return {m.column(I)...};
}
}

template <size_t Rows, size_t Columns, typename T>
constexpr Matrix<Columns, Rows, T> transpose(const Matrix<Rows, Columns, T>& m)
{
        return matrix_implementation::transpose_impl(m, std::make_integer_sequence<size_t, Columns>());
}

namespace matrix_implementation
{
template <size_t Rows, size_t Columns, typename Dst, typename Src, size_t... I>
Matrix<Rows, Columns, Dst> to_matrix(const Matrix<Rows, Columns, Src>& m, std::integer_sequence<size_t, I...>)
{
        static_assert(sizeof...(I) == Rows);
        static_assert(((I < Rows) && ...));
        static_assert(!std::is_same_v<std::remove_cv_t<Dst>, std::remove_cv_t<Src>>);

        return {to_vector<Dst>(m[I])...};
}
}

template <typename Dst, size_t Rows, size_t Columns, typename Src, size_t... I>
Matrix<Rows, Columns, Dst> to_matrix(const Matrix<Rows, Columns, Src>& m)
{
        return matrix_implementation::to_matrix<Rows, Columns, Dst, Src>(m, std::make_integer_sequence<size_t, Rows>());
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
