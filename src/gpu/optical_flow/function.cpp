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

#include "function.h"

// #include <src/com/log.h>
// #include <src/com/print.h>

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/group_count.h>

namespace ns::gpu::optical_flow
{
std::vector<Vector2i> pyramid_sizes(int width, int height, const int min_size)
{
        std::vector<Vector2i> sizes;

        sizes.emplace_back(width, height);

        while (true)
        {
                int new_width = (width + 1) / 2;
                int new_height = (height + 1) / 2;

                if (new_width < min_size)
                {
                        new_width = width;
                }
                if (new_height < min_size)
                {
                        new_height = height;
                }

                if (new_width == width && new_height == height)
                {
                        break;
                }

                sizes.emplace_back(new_width, new_height);

                width = new_width;
                height = new_height;
        }

        // for (const Vector2i& v : sizes)
        // {
        //         LOG(to_string(v[0]) + " x " + to_string(v[1]));
        // }

        return sizes;
}

std::vector<Vector2i> flow_groups(
        const Vector2i& group_size,
        const std::vector<Vector2i>& sizes,
        const Vector2i& top_point_count)
{
        std::vector<Vector2i> groups;
        groups.reserve(sizes.size());

        groups.push_back(group_count(top_point_count, group_size));

        for (std::size_t i = 1; i < sizes.size(); ++i)
        {
                groups.push_back(group_count(sizes[i], group_size));
        }

        return groups;
}

TopLevelPoints create_top_level_points(
        const int width,
        const int height,
        const double distance_between_points_in_mm,
        const double ppi)
{
        ASSERT(width >= 0 && height >= 0 && ppi >= 0);

        const int distance = millimeters_to_pixels(distance_between_points_in_mm, ppi);

        if (width <= 0 || height <= 0 || distance < 0)
        {
                return {.count_x = 0, .count_y = 0, .points = {}};
        }

        const int lw = width - 2 * distance;
        const int lh = height - 2 * distance;

        if (lw <= 0 || lh <= 0)
        {
                return {.count_x = 0, .count_y = 0, .points = {}};
        }

        const int size = distance + 1;
        const int count_x = (lw + size - 1) / size;
        const int count_y = (lh + size - 1) / size;
        const long long point_count = static_cast<long long>(count_x) * count_y;

        std::vector<Vector2i> points(point_count);
        long long index = 0;
        for (int y = distance; y < height - distance; y += size)
        {
                for (int x = distance; x < width - distance; x += size)
                {
                        points[index++] = Vector2i(x, y);
                }
        }

        ASSERT(index == point_count);
        ASSERT(static_cast<std::size_t>(count_x) * count_y == points.size());

        return {.count_x = count_x, .count_y = count_y, .points = std::move(points)};
}
}
