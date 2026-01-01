/*
Copyright (C) 2017-2026 Topological Manifold

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

#pragma once

#include <vulkan/vulkan_core.h>

namespace ns::vulkan::buffers
{
VkExtent3D find_max_image_extent(
        VkPhysicalDevice physical_device,
        VkFormat format,
        VkImageType image_type,
        VkImageTiling tiling,
        VkImageUsageFlags usage);

VkExtent3D correct_image_extent(VkImageType type, VkExtent3D extent);

VkExtent3D limit_image_extent(
        VkImageType type,
        VkExtent3D extent,
        VkPhysicalDevice physical_device,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage);

void transition_image_layout(
        VkImageAspectFlags aspect_flags,
        VkDevice device,
        VkCommandPool command_pool,
        VkQueue queue,
        VkImage image,
        VkImageLayout layout);

VkFormatFeatureFlags format_features_for_image_usage(VkImageUsageFlags usage);

bool has_usage_for_image_view(VkImageUsageFlags usage);

bool has_usage_for_transfer(VkImageUsageFlags usage);
}
