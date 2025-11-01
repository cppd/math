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

#include "sample.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <flat_set>

namespace ns::vulkan
{
namespace
{
bool set(const VkSampleCountFlags flags, const VkSampleCountFlagBits bits)
{
        return (flags & bits) == bits;
}
}

std::flat_set<int> supported_sample_counts(const VkPhysicalDeviceLimits& limits)
{
        const VkSampleCountFlags flags =
                limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts
                & limits.storageImageSampleCounts;

        std::flat_set<int> res;

        const auto insert = [&](const VkSampleCountFlagBits bits)
        {
                if (set(flags, bits))
                {
                        res.insert(sample_count_flag_to_sample_count(bits));
                }
        };

        insert(VK_SAMPLE_COUNT_1_BIT);
        insert(VK_SAMPLE_COUNT_2_BIT);
        insert(VK_SAMPLE_COUNT_4_BIT);
        insert(VK_SAMPLE_COUNT_8_BIT);
        insert(VK_SAMPLE_COUNT_16_BIT);
        insert(VK_SAMPLE_COUNT_32_BIT);
        insert(VK_SAMPLE_COUNT_64_BIT);

        if (res.empty())
        {
                error("Sample counts not found");
        }

        return res;
}

VkSampleCountFlagBits sample_count_to_sample_count_flag(const int sample_count)
{
        switch (sample_count)
        {
        case 1:
                return VK_SAMPLE_COUNT_1_BIT;
        case 2:
                return VK_SAMPLE_COUNT_2_BIT;
        case 4:
                return VK_SAMPLE_COUNT_4_BIT;
        case 8:
                return VK_SAMPLE_COUNT_8_BIT;
        case 16:
                return VK_SAMPLE_COUNT_16_BIT;
        case 32:
                return VK_SAMPLE_COUNT_32_BIT;
        case 64:
                return VK_SAMPLE_COUNT_64_BIT;
        default:
                error("Unsupported sample count " + to_string(sample_count));
        }
}

int sample_count_flag_to_sample_count(const VkSampleCountFlagBits sample_count)
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
