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

#include "point_clipping_behavior.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan
{
std::string point_clipping_behavior_to_string(const VkPointClippingBehavior point_clipping_behavior)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (point_clipping_behavior)
        {
                CASE(VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES)
                CASE(VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY)
        }
#pragma GCC diagnostic pop

        return "Unknown VkPointClippingBehavior " + to_string(enum_to_int(point_clipping_behavior));
}
}
