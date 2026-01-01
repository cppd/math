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

#include <array>
#include <vector>

namespace ns::gpu::optical_flow
{
std::vector<std::array<int, 2>> pyramid_sizes(int width, int height, int min_size);

struct TopLevelPoints final
{
        int count_x;
        int count_y;
        std::vector<std::array<int, 2>> points;
};

TopLevelPoints create_top_level_points(int width, int height, double distance_between_points_in_mm, double ppi);
}
