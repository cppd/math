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

#include <src/com/math.h>
#include <src/vulkan/layout.h>

#include <cstddef>

namespace ns::vulkan
{
namespace
{
template <typename T>
struct Test;

template <std::size_t N, typename T>
struct Test<std140::Matrix<N, T>> final
{
        static constexpr std::size_t ALIGNMENT = (N != 3 ? N : 4) * sizeof(T);
        static_assert(sizeof(std140::Matrix<N, T>) == N * round_up(ALIGNMENT, 4 * sizeof(float)));
        static_assert(alignof(std140::Matrix<N, T>) == round_up(ALIGNMENT, 4 * sizeof(float)));
};

template struct Test<std140::Matrix2f>;
template struct Test<std140::Matrix3f>;
template struct Test<std140::Matrix4f>;

template struct Test<std140::Matrix2d>;
template struct Test<std140::Matrix3d>;
template struct Test<std140::Matrix4d>;

//

static_assert(sizeof(std140::Vector2f) == 2 * sizeof(float));
static_assert(alignof(std140::Vector2f) == 2 * sizeof(float));
static_assert(sizeof(std140::Vector3f) == 3 * sizeof(float));
static_assert(alignof(std140::Vector3f) == 4 * sizeof(float));
static_assert(sizeof(std140::Vector4f) == 4 * sizeof(float));
static_assert(alignof(std140::Vector4f) == 4 * sizeof(float));

static_assert(sizeof(std140::Vector2d) == 2 * sizeof(double));
static_assert(alignof(std140::Vector2d) == 2 * sizeof(double));
static_assert(sizeof(std140::Vector3d) == 3 * sizeof(double));
static_assert(alignof(std140::Vector3d) == 4 * sizeof(double));
static_assert(sizeof(std140::Vector4d) == 4 * sizeof(double));
static_assert(alignof(std140::Vector4d) == 4 * sizeof(double));

struct TestFloat final
{
        std140::Vector3f v;
        float f;
};

static_assert(sizeof(TestFloat) == 4 * sizeof(float));
static_assert(alignof(TestFloat) == 4 * sizeof(float));

struct TestDouble final
{
        std140::Vector3d v;
        double d;
};

static_assert(sizeof(TestDouble) == 4 * sizeof(double));
static_assert(alignof(TestDouble) == 4 * sizeof(double));
}
}
