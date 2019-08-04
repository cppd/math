/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/math.h"

namespace gpgpu_convex_hull_show_implementation
{
// rad / ms
constexpr double ANGULAR_FREQUENCY = TWO_PI<double> * 5;

inline int points_buffer_size(int height)
{
        // 2 линии точек + 1 точка, тип ivec2
        return (2 * height + 1) * (2 * sizeof(int32_t));
}
}

namespace gpgpu_convex_hull_compute_implementation
{
int group_size_prepare(int width, unsigned max_group_size_x, unsigned max_group_invocations, unsigned max_shared_memory_size);
int group_size_merge(int height, unsigned max_group_size_x, unsigned max_group_invocations, unsigned max_shared_memory_size);
int iteration_count_merge(int size);
}
