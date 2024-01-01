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

#include <array>
#include <cstddef>

namespace ns
{
template <typename T>
[[nodiscard]] constexpr T group_count(const T& size, const T& group_size)
{
        return (size + group_size - T{1}) / group_size;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr std::array<T, N> group_count(const std::array<T, N>& sizes, const std::array<T, N>& group_sizes)
{
        std::array<T, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = group_count(sizes[i], group_sizes[i]);
        }
        return res;
}
}
