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

#include "image_layout.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <string>
#include <vulkan/vulkan_core.h>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan
{
std::string image_layout_to_string(const VkImageLayout image_layout)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (image_layout)
        {
                CASE(VK_IMAGE_LAYOUT_UNDEFINED)
                CASE(VK_IMAGE_LAYOUT_GENERAL)
                CASE(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_PREINITIALIZED)
                CASE(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL)
                CASE(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                CASE(VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR)
                CASE(VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT)
                CASE(VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR)
                CASE(VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT)
        }
#pragma GCC diagnostic pop

        return "Unknown VkImageLayout " + to_string(enum_to_int(image_layout));
}
}
