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

#include "type/limit.h"

#include <cmath>
#include <type_traits>

namespace ns
{
template <typename T>
[[nodiscard]] constexpr T absolute(const T& v)
{
        constexpr bool HAS_ABS = requires { std::abs(v); };
        if constexpr (HAS_ABS)
        {
                if (!std::is_constant_evaluated())
                {
                        return std::abs(v);
                }
        }
        return v < 0 ? -v : v;
}

template <typename T>
        requires std::is_floating_point_v<T>
[[nodiscard]] constexpr bool is_finite(const T& v)
{
        if (std::is_constant_evaluated())
        {
                return v >= Limits<T>::lowest() && v <= Limits<T>::max();
        }
        return std::isfinite(v);
}

template <typename I, typename T>
[[nodiscard]] constexpr I integral_floor(const T& v)
{
        static_assert(std::is_integral_v<I> && std::is_signed_v<I>);
        static_assert(std::is_floating_point_v<T>);
        const I i = v;
        return v < i ? i - 1 : i;
}

template <typename I, typename T>
[[nodiscard]] constexpr I integral_ceil(const T& v)
{
        static_assert(std::is_integral_v<I> && std::is_signed_v<I>);
        static_assert(std::is_floating_point_v<T>);
        const I i = v;
        return v > i ? i + 1 : i;
}

template <typename T>
[[nodiscard]] constexpr T round_up(const T v, const T to)
{
        static_assert(std::is_unsigned_v<T>);
        return ((v + to - 1) / to) * to;
}
}
