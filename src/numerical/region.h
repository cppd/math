/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "vector.h"

#include <src/com/error.h>

#include <cstddef>

namespace ns::numerical
{
template <std::size_t N, typename T>
class Region final
{
        static_assert(N >= 1);
        static_assert(std::is_integral_v<T>);

        Vector<N, T> offset0_;
        Vector<N, T> extent_;
        Vector<N, T> offset1_;

public:
        constexpr Region()
        {
        }

        constexpr Region(const Vector<N, T>& offset, const Vector<N, T>& extent)
                : offset0_(offset),
                  extent_(extent),
                  offset1_(offset0_ + extent_)
        {
        }

        [[nodiscard]] constexpr const Vector<N, T>& from() const
        {
                return offset0_;
        }

        [[nodiscard]] constexpr const Vector<N, T>& to() const
        {
                return offset1_;
        }

        [[nodiscard]] constexpr const Vector<N, T>& extent() const
        {
                return extent_;
        }

        [[nodiscard]] constexpr T x0() const
                requires (N == 1 || N == 2 || N == 3)
        {
                return offset0_[0];
        }

        [[nodiscard]] constexpr T y0() const
                requires (N == 2 || N == 3)
        {
                return offset0_[1];
        }

        [[nodiscard]] constexpr T z0() const
                requires (N == 3)
        {
                return offset0_[2];
        }

        [[nodiscard]] constexpr T x1() const
                requires (N == 1 || N == 2 || N == 3)
        {
                return offset1_[0];
        }

        [[nodiscard]] constexpr T y1() const
                requires (N == 2 || N == 3)
        {
                return offset1_[1];
        }

        [[nodiscard]] constexpr T z1() const
                requires (N == 3)
        {
                return offset1_[2];
        }

        [[nodiscard]] constexpr T width() const
                requires (N == 1 || N == 2 || N == 3)
        {
                return extent_[0];
        }

        [[nodiscard]] constexpr T height() const
                requires (N == 2 || N == 3)
        {
                return extent_[1];
        }

        [[nodiscard]] constexpr T depth() const
                requires (N == 3)
        {
                return extent_[2];
        }

        template <typename Type>
        [[nodiscard]] constexpr bool is_inside(const Vector<N, Type>& p) const
        {
                static_assert(std::is_integral_v<Type>);

                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(p[i] >= offset0_[i] && p[i] < offset1_[i]))
                        {
                                return false;
                        }
                }
                return true;
        }

        template <typename... Types>
        [[nodiscard]] constexpr bool is_inside(const Types&... p) const
        {
                static_assert(sizeof...(Types) == N);
                static_assert((std::is_integral_v<Types> && ...));

                int i = -1;
                return (((++i, p >= offset0_[i] && p < offset1_[i])) && ...);
        }

        [[nodiscard]] constexpr bool is_positive() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (offset0_[i] < 0)
                        {
                                return false;
                        }
                        if (extent_[i] <= 0)
                        {
                                return false;
                        }
                        ASSERT(offset0_[i] + extent_[i] == offset1_[i]);
                }
                return true;
        }
};
}
