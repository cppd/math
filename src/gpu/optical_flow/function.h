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

#include <src/numerical/vec.h>

#include <vector>

namespace ns::gpu::optical_flow
{
std::vector<vec2i> pyramid_sizes(int width, int height, int min_size);

vec2i grayscale_groups(const vec2i& group_size, const std::vector<vec2i>& sizes);

std::vector<vec2i> downsample_groups(const vec2i& group_size, const std::vector<vec2i>& sizes);

std::vector<vec2i> sobel_groups(const vec2i& group_size, const std::vector<vec2i>& sizes);

std::vector<vec2i> flow_groups(
        const vec2i& group_size,
        const std::vector<vec2i>& sizes,
        int top_point_count_x,
        int top_point_count_y);

void create_top_level_points(
        int width,
        int height,
        double distance_between_points_in_mm,
        int ppi,
        int* point_count_x,
        int* point_count_y,
        std::vector<vec2i>* points);
}
