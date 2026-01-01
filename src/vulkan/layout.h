/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/math.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::vulkan
{
namespace std140
{
namespace implementation
{
template <std::size_t N, typename T>
inline constexpr std::size_t VECTOR_ALIGNMENT = (N != 3 ? N : 4) * sizeof(T);

template <std::size_t N, typename T>
inline constexpr std::size_t BASE_ALIGNMENT = round_up(VECTOR_ALIGNMENT<N, T>, VECTOR_ALIGNMENT<4, float>);
}

template <std::size_t N, typename T>
        requires (N >= 2 && N <= 4 && (std::is_same_v<T, float> || std::is_same_v<T, double>))
struct Matrix final
{
        struct alignas(implementation::BASE_ALIGNMENT<N, T>) Column final
        {
                std::array<T, N> data;
        };

        std::array<Column, N> columns;
};

using Matrix2f = Matrix<2, float>;
using Matrix3f = Matrix<3, float>;
using Matrix4f = Matrix<4, float>;

using Matrix2d = Matrix<2, double>;
using Matrix3d = Matrix<3, double>;
using Matrix4d = Matrix<4, double>;

typedef __attribute__((aligned(implementation::VECTOR_ALIGNMENT<2, float>))) numerical::Vector2f Vector2f;
typedef __attribute__((aligned(implementation::VECTOR_ALIGNMENT<3, float>))) numerical::Vector3f Vector3f;
typedef __attribute__((aligned(implementation::VECTOR_ALIGNMENT<4, float>))) numerical::Vector4f Vector4f;

typedef __attribute__((aligned(implementation::VECTOR_ALIGNMENT<2, double>))) numerical::Vector2d Vector2d;
typedef __attribute__((aligned(implementation::VECTOR_ALIGNMENT<3, double>))) numerical::Vector3d Vector3d;
typedef __attribute__((aligned(implementation::VECTOR_ALIGNMENT<4, double>))) numerical::Vector4d Vector4d;
}

template <typename Dst, std::size_t N, typename Src>
[[nodiscard]] constexpr std140::Matrix<N, Dst> to_std140(const numerical::Matrix<N, N, Src>& m)
{
        std140::Matrix<N, Dst> res;
        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t c = 0; c < N; ++c)
                {
                        res.columns[c].data[r] = m[r, c];
                }
        }
        return res;
}
}
