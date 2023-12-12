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

#include <src/numerical/vector.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace ns::model::mesh::file::stl
{
template <typename T>
        requires std::is_same_v<std::uint32_t, T>
[[nodiscard]] constexpr T byte_swap(const T n)
{
        static_assert(__builtin_bswap32(0x12345678) == 0x78563412);
        static_assert(__builtin_bswap32(0x78563412) == 0x12345678);
        return __builtin_bswap32(n);
}

template <std::size_t N>
[[nodiscard]] std::array<Vector<N, float>, N> byte_swap(const std::array<Vector<N, std::uint32_t>, N>& facet_vertices)
{
        std::array<Vector<N, float>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = 0; j < N; ++j)
                {
                        res[i][j] = std::bit_cast<float>(byte_swap(facet_vertices[i][j]));
                }
        }
        return res;
}

template <std::size_t N>
[[nodiscard]] Vector<N, std::uint32_t> byte_swap(const Vector<N, float>& v)
{
        Vector<N, std::uint32_t> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = byte_swap(std::bit_cast<std::uint32_t>(v[i]));
        }
        return res;
}
}
