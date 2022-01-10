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

#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
VkExtent3D find_max_image_extent(
        VkPhysicalDevice physical_device,
        VkFormat format,
        VkImageType image_type,
        VkImageTiling tiling,
        VkImageUsageFlags usage);

VkSampleCountFlagBits supported_color_depth_framebuffer_sample_count_flag(
        VkPhysicalDevice physical_device,
        int required_minimum_sample_count);

int sample_count_flag_to_integer(VkSampleCountFlagBits sample_count);

std::uint32_t physical_device_memory_type_index(
        VkPhysicalDevice physical_device,
        std::uint32_t memory_type_bits,
        VkMemoryPropertyFlags memory_property_flags);
}
