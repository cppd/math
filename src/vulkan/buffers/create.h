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

#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::vulkan::buffers
{
Buffer create_buffer(
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        std::vector<std::uint32_t> family_indices);

Image create_image(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkImageType type,
        VkExtent3D extent,
        VkFormat format,
        std::vector<std::uint32_t> family_indices,
        VkSampleCountFlagBits samples,
        VkImageTiling tiling,
        VkImageUsageFlags usage);

ImageView create_image_view(const Image& image, VkImageAspectFlags aspect_flags);
}
