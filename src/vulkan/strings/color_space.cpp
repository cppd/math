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

#include "color_space.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan::strings
{
std::string color_space_to_string(const VkColorSpaceKHR color_space)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (color_space)
        {
                CASE(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                CASE(VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT)
                CASE(VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)
                CASE(VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT)
                CASE(VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT)
                CASE(VK_COLOR_SPACE_BT709_LINEAR_EXT)
                CASE(VK_COLOR_SPACE_BT709_NONLINEAR_EXT)
                CASE(VK_COLOR_SPACE_BT2020_LINEAR_EXT)
                CASE(VK_COLOR_SPACE_HDR10_ST2084_EXT)
                CASE(VK_COLOR_SPACE_DOLBYVISION_EXT)
                CASE(VK_COLOR_SPACE_HDR10_HLG_EXT)
                CASE(VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT)
                CASE(VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT)
                CASE(VK_COLOR_SPACE_PASS_THROUGH_EXT)
                CASE(VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT)
                CASE(VK_COLOR_SPACE_DISPLAY_NATIVE_AMD)
        }
#pragma GCC diagnostic pop

        return "Unknown VkColorSpaceKHR " + to_string(enum_to_int(color_space));
}
}
