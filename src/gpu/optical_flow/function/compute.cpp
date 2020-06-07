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

#include "compute.h"

//#include <src/com/log.h>
//#include <src/com/print.h>

#include "../../com/groups.h"

namespace gpu::optical_flow
{
std::vector<vec2i> pyramid_sizes(int width, int height, int min_size)
{
        std::vector<vec2i> sizes;

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

        // for (const vec2i& v : sizes)
        //{
        //        LOG(to_string(v[0]) + " x " + to_string(v[1]));
        //}

        return sizes;
}

vec2i grayscale_groups(const vec2i& group_size, const std::vector<vec2i>& sizes)
{
        return group_count(sizes[0][0], sizes[0][1], group_size);
}

std::vector<vec2i> downsample_groups(const vec2i& group_size, const std::vector<vec2i>& sizes)
{
        std::vector<vec2i> groups;

        for (unsigned i = 1; i < sizes.size(); ++i)
        {
                groups.push_back(group_count(sizes[i][0], sizes[i][1], group_size));
        }

        return groups;
}

std::vector<vec2i> sobel_groups(const vec2i& group_size, const std::vector<vec2i>& sizes)
{
        std::vector<vec2i> groups;
        groups.reserve(sizes.size());

        for (const vec2i& size : sizes)
        {
                groups.push_back(group_count(size[0], size[1], group_size));
        }

        return groups;
}

std::vector<vec2i> flow_groups(
        const vec2i& group_size,
        const std::vector<vec2i>& sizes,
        int top_point_count_x,
        int top_point_count_y)
{
        std::vector<vec2i> groups;

        groups.push_back(group_count(top_point_count_x, top_point_count_y, group_size));

        for (size_t i = 1; i < sizes.size(); ++i)
        {
                groups.push_back(group_count(sizes[i][0], sizes[i][1], group_size));
        }

        return groups;
}
}
