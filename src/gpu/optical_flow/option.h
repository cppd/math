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

namespace ns::gpu::optical_flow
{
inline constexpr numerical::Vector2i GROUP_SIZE(16, 16);

inline constexpr int BOTTOM_IMAGE_MINIMUM_SIZE = 16;

inline constexpr int RADIUS = 6;
inline constexpr int MAX_ITERATION_COUNT = 10;
inline constexpr float STOP_MOVE_SQUARE = square(1e-3f);
inline constexpr float MIN_DETERMINANT = 1;

inline constexpr double DISTANCE_BETWEEN_POINTS_IN_MM = 2;
}
