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

#include <src/com/exponent.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <type_traits>

namespace ns::painter::lights::com
{
// power<N - 1>(distance)
template <std::size_t N, typename T>
[[nodiscard]] T power_n1(const T squared_distance, const T distance)
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 1);

        if constexpr ((N & 1) == 1)
        {
                return power<((N - 1) / 2)>(squared_distance);
        }
        else
        {
                return power<((N - 2) / 2)>(squared_distance) * distance;
        }
}

template <std::size_t N, std::size_t M, typename T>
std::array<numerical::Vector<N, T>, M> multiply(const std::array<numerical::Vector<N, T>, M>& vectors, const T value)
{
        std::array<numerical::Vector<N, T>, M> res;
        for (std::size_t i = 0; i < M; ++i)
        {
                res[i] = vectors[i] * value;
        }
        return res;
}
}
