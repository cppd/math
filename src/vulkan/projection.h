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

#include <src/numerical/matrix.h>

namespace ns::vulkan
{
// Right-handed coordinate systems
// X to the right [-1, 1], Y downward [-1, 1], Z [0, 1]
[[nodiscard]] constexpr numerical::Matrix4d orthographic_projection(
        const double left,
        const double right,
        const double bottom,
        const double top,
        const double near,
        const double far)
{
        const double w = right - left;
        const double h = bottom - top;
        const double d = far - near;

        return {
                {2 / w,     0,     0, -(right + left) / w},
                {    0, 2 / h,     0, -(bottom + top) / h},
                {    0,     0, 1 / d,           -near / d},
                {    0,     0,     0,                   1},
        };
}
}
