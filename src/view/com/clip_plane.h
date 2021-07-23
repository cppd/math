/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>

namespace ns::view
{
[[nodiscard]] inline vec4d create_clip_plane(const mat4d& clip_plane_view, const double position)
{
        ASSERT(position >= 0.0 && position <= 1.0);

        // Уравнение плоскости
        // -z = 0 или (0, 0, -1, 0).
        // Уравнение плоскости для исходных координат
        // (0, 0, -1, 0) * view matrix.
        vec4d plane = -clip_plane_view.row(2);

        vec3d n(plane[0], plane[1], plane[2]);
        double d = n.norm_1();

        // Уравнение плоскости со смещением
        // -z = d * (1 - 2 * position) или (0, 0, -1, d * (2 * position - 1)).
        plane[3] += d * (2 * position - 1);

        plane /= n.norm();

        return plane;
}
}
