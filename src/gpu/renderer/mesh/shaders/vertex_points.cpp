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

#include "vertex_points.h"

#include <cstddef>
#include <vector>

namespace ns::gpu::renderer
{
std::vector<VkVertexInputBindingDescription> PointsVertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;
        descriptions.reserve(1);

        descriptions.push_back(
                {.binding = 0, .stride = sizeof(PointsVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> PointsVertex::attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;
        descriptions.reserve(1);

        descriptions.push_back(
                {.location = 0,
                 .binding = 0,
                 .format = VK_FORMAT_R32G32B32_SFLOAT,
                 .offset = offsetof(PointsVertex, position)});

        return descriptions;
}
}
