/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "show.h"

#include "com/conversion.h"
#include "com/error.h"

// Расстояние между точками потока на экране в миллиметрах
constexpr double DISTANCE_BETWEEN_POINTS = 2;

void create_top_level_optical_flow_points(
        int width,
        int height,
        int ppi,
        int* point_count_x,
        int* point_count_y,
        std::vector<vec2i>* points)
{
        ASSERT(width >= 0 && height >= 0 && ppi >= 0);

        points->clear();

        int distance = millimeters_to_pixels(DISTANCE_BETWEEN_POINTS, ppi);

        if (width <= 0 || height <= 0 || distance < 0)
        {
                *point_count_x = 0;
                *point_count_y = 0;
                return;
        }

        int lw = width - 2 * distance;
        int lh = height - 2 * distance;

        if (lw <= 0 || lh <= 0)
        {
                *point_count_x = 0;
                *point_count_y = 0;
                return;
        }

        int size = distance + 1;
        *point_count_x = (lw + size - 1) / size;
        *point_count_y = (lh + size - 1) / size;

        int point_count = *point_count_x * *point_count_y;

        points->clear();
        points->resize(point_count);

        int index = 0;
        for (int y = distance; y < height - distance; y += size)
        {
                for (int x = distance; x < width - distance; x += size)
                {
                        (*points)[index++] = vec2i(x, y);
                }
        }

        ASSERT(index == point_count);
        ASSERT(static_cast<size_t>(*point_count_x) * *point_count_y == points->size());
}
