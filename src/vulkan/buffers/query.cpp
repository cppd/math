/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "query.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/vulkan/error.h>
#include <src/vulkan/strings.h>

#include <vulkan/vulkan_core.h>

#include <ios>
#include <sstream>
#include <vector>

namespace ns::vulkan::buffers
{
VkFormat find_supported_format(
        const VkPhysicalDevice device,
        const std::vector<VkFormat>& candidates,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features)
{
        if (tiling == VK_IMAGE_TILING_OPTIMAL)
        {
                for (const VkFormat format : candidates)
                {
                        VkFormatProperties properties;
                        vkGetPhysicalDeviceFormatProperties(device, format, &properties);
                        if ((properties.optimalTilingFeatures & features) == features)
                        {
                                return format;
                        }
                }
        }
        else if (tiling == VK_IMAGE_TILING_LINEAR)
        {
                for (const VkFormat format : candidates)
                {
                        VkFormatProperties properties;
                        vkGetPhysicalDeviceFormatProperties(device, format, &properties);
                        if ((properties.linearTilingFeatures & features) == features)
                        {
                                return format;
                        }
                }
        }
        else
        {
                error("Unknown image tiling " + to_string(enum_to_int(tiling)));
        }

        std::ostringstream oss;

        oss << "Failed to find supported 2D image format.";
        oss << " Format candidates " << strings::formats_to_sorted_string(candidates, ", ") << ".";
        oss << " Tiling " << enum_to_int(tiling) << ".";
        oss << std::hex;
        oss << " Features 0x" << features << ".";

        error(oss.str());
}

VkFormat find_supported_image_format(
        const VkPhysicalDevice device,
        const std::vector<VkFormat>& candidates,
        const VkImageType image_type,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features,
        const VkImageUsageFlags usage,
        const VkSampleCountFlags sample_count)
{
        for (const VkFormat format : candidates)
        {
                VkFormatProperties properties;
                vkGetPhysicalDeviceFormatProperties(device, format, &properties);

                if (tiling == VK_IMAGE_TILING_OPTIMAL)
                {
                        if ((properties.optimalTilingFeatures & features) != features)
                        {
                                continue;
                        }
                }
                else if (tiling == VK_IMAGE_TILING_LINEAR)
                {
                        if ((properties.linearTilingFeatures & features) != features)
                        {
                                continue;
                        }
                }
                else
                {
                        error("Unknown image tiling " + to_string(enum_to_int(tiling)));
                }

                VkImageFormatProperties image_properties;
                VULKAN_CHECK(vkGetPhysicalDeviceImageFormatProperties(
                        device, format, image_type, tiling, usage, 0 /*VkImageCreateFlags*/, &image_properties));

                if ((image_properties.sampleCounts & sample_count) != sample_count)
                {
                        continue;
                }

                return format;
        }

        std::ostringstream oss;

        oss << "Failed to find supported image format.";
        oss << " Format candidates " << strings::formats_to_sorted_string(candidates, ", ") << ".";
        oss << " Image type " << strings::image_type_to_string(image_type) << ".";
        oss << " Tiling " << enum_to_int(tiling) << ".";
        oss << std::hex;
        oss << " Features 0x" << features << ".";
        oss << " Usage 0x" << usage << ".";
        oss << " Sample count 0x" << sample_count << ".";

        error(oss.str());
}
}
