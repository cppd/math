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

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace ns
{
template <std::size_t ROWS, std::size_t COLUMNS, typename T>
class Matrix final
{
        static_assert(FloatingPoint<T>);
        static_assert(ROWS >= 1 && COLUMNS >= 1);

        std::array<Vector<COLUMNS, T>, ROWS> rows_;

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

        explicit constexpr Matrix(const std::type_identity_t<T> value)
        {
                for (std::size_t r = 0; r < ROWS; ++r)
                {
                        for (std::size_t c = 0; c < COLUMNS; ++c)
                        {
                                rows_[r][c] = value;
                        }
                }
        }

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

        explicit constexpr Matrix(const Vector<ROWS == 1 ? COLUMNS : ROWS, T>& data)
                requires (ROWS == 1 || COLUMNS == 1)
        {
                if constexpr (ROWS == 1)
                {
                        rows_[0] = data;
                }
                else
                {
                        for (std::size_t i = 0; i < ROWS; ++i)
                        {
                                rows_[i][0] = data[i];
                        }
                }
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

        [[nodiscard]] constexpr Vector<ROWS, T> column(const std::size_t column) const
        {
                return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
                {
                        static_assert(sizeof...(I) == ROWS);
                        static_assert(((I >= 0 && I < ROWS) && ...));

                        return Vector<ROWS, T>{rows_[I][column]...};
                }(std::make_integer_sequence<std::size_t, ROWS>());
        }

        [[nodiscard]] constexpr const T& operator()(const std::size_t r, const std::size_t c) const
        {
                return rows_[r][c];
        }

        [[nodiscard]] constexpr T& operator()(const std::size_t r, const std::size_t c)
        {
                return rows_[r][c];
        }

        [[nodiscard]] constexpr const T& operator()(const std::size_t index) const
                requires (ROWS == 1 || COLUMNS == 1)
        {
                if constexpr (ROWS == 1)
                {
                        return rows_[0][index];
                }
                else
                {
                        return rows_[index][0];
                }
        }

        [[nodiscard]] constexpr T& operator()(const std::size_t index)
                requires (ROWS == 1 || COLUMNS == 1)
        {
                if constexpr (ROWS == 1)
                {
                        return rows_[0][index];
                }
                else
                {
                        return rows_[index][0];
                }
        }

        [[nodiscard]] constexpr Vector<ROWS == 1 ? COLUMNS : ROWS, T> to_vector() const
                requires (ROWS == 1 || COLUMNS == 1)
        {
                if constexpr (ROWS == 1)
                {
                        return rows_[0];
                }
                else
                {
                        return column(0);
                }
        }

        [[nodiscard]] constexpr Matrix<COLUMNS, ROWS, T> transposed() const
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

        [[nodiscard]] Matrix<ROWS, ROWS, T> inversed() const
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
        [[nodiscard]] constexpr Matrix<R, C, T> top_left() const
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

        [[nodiscard]] constexpr T trace() const
        {
                constexpr std::size_t N = std::min(ROWS, COLUMNS);

                T res = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res += rows_[i][i];
                }
                return res;
        }

        [[nodiscard]] constexpr Vector<std::min(ROWS, COLUMNS), T> diagonal() const
        {
                constexpr std::size_t N = std::min(ROWS, COLUMNS);

                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = rows_[i][i];
                }
                return res;
        }

        [[nodiscard]] bool is_finite() const
                requires (std::is_floating_point_v<T>)
        {
                for (std::size_t i = 0; i < ROWS; ++i)
                {
                        if (rows_[i].is_finite())
                        {
                                continue;
                        }
                        return false;
                }
                return true;
        }
};

using Matrix3d = Matrix<3, 3, double>;
using Matrix3f = Matrix<3, 3, float>;
using Matrix4d = Matrix<4, 4, double>;
using Matrix4f = Matrix<4, 4, float>;
}
