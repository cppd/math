/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "primitive_topology.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan::strings
{
std::string primitive_topology_to_string(const VkPrimitiveTopology primitive_topology)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (primitive_topology)
        {
                CASE(VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
                CASE(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
                CASE(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP)
                CASE(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                CASE(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
                CASE(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN)
                CASE(VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY)
                CASE(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY)
                CASE(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY)
                CASE(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY)
                CASE(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)
        }
#pragma GCC diagnostic pop

        return "Unknown VkPrimitiveTopology " + to_string(enum_to_int(primitive_topology));
}
}
