/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "sample.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>

namespace ns::vulkan
{
namespace
{
bool set(const VkSampleCountFlags flags, const VkSampleCountFlagBits bits)
{
        return (flags & bits) == bits;
}
}

VkSampleCountFlagBits supported_color_depth_framebuffer_sample_count_flag(
        const VkPhysicalDevice physical_device,
        const int required_minimum_sample_count)
{
        constexpr int MIN_SAMPLE_COUNT = 1;
        constexpr int MAX_SAMPLE_COUNT = 64;

        if (required_minimum_sample_count < MIN_SAMPLE_COUNT)
        {
                error("The required minimum sample count " + to_string(required_minimum_sample_count) + " is less than "
                      + to_string(MIN_SAMPLE_COUNT));
        }
        if (required_minimum_sample_count > MAX_SAMPLE_COUNT)
        {
                error("The required minimum sample count " + to_string(required_minimum_sample_count)
                      + " is greater than " + to_string(MAX_SAMPLE_COUNT));
        }

        const VkSampleCountFlags flags = [&]
        {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(physical_device, &properties);
                return properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
        }();

        if ((required_minimum_sample_count <= 1) && set(flags, VK_SAMPLE_COUNT_1_BIT))
        {
                return VK_SAMPLE_COUNT_1_BIT;
        }

        if ((required_minimum_sample_count <= 2) && set(flags, VK_SAMPLE_COUNT_2_BIT))
        {
                return VK_SAMPLE_COUNT_2_BIT;
        }

        if ((required_minimum_sample_count <= 4) && set(flags, VK_SAMPLE_COUNT_4_BIT))
        {
                return VK_SAMPLE_COUNT_4_BIT;
        }

        if ((required_minimum_sample_count <= 8) && set(flags, VK_SAMPLE_COUNT_8_BIT))
        {
                return VK_SAMPLE_COUNT_8_BIT;
        }

        if ((required_minimum_sample_count <= 16) && set(flags, VK_SAMPLE_COUNT_16_BIT))
        {
                return VK_SAMPLE_COUNT_16_BIT;
        }

        if ((required_minimum_sample_count <= 32) && set(flags, VK_SAMPLE_COUNT_32_BIT))
        {
                return VK_SAMPLE_COUNT_32_BIT;
        }

        if ((required_minimum_sample_count <= 64) && set(flags, VK_SAMPLE_COUNT_64_BIT))
        {
                return VK_SAMPLE_COUNT_64_BIT;
        }

        error("The required minimum sample count " + to_string(required_minimum_sample_count) + " is not available");
}

int sample_count_flag_to_integer(const VkSampleCountFlagBits sample_count)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (sample_count)
        {
        case VK_SAMPLE_COUNT_1_BIT:
                return 1;
        case VK_SAMPLE_COUNT_2_BIT:
                return 2;
        case VK_SAMPLE_COUNT_4_BIT:
                return 4;
        case VK_SAMPLE_COUNT_8_BIT:
                return 8;
        case VK_SAMPLE_COUNT_16_BIT:
                return 16;
        case VK_SAMPLE_COUNT_32_BIT:
                return 32;
        case VK_SAMPLE_COUNT_64_BIT:
                return 64;
        }
#pragma GCC diagnostic pop

        error("Unknown sample count flag " + to_string(enum_to_int(sample_count)));
}
}
