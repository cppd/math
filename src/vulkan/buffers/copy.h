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

#pragma once

#include "../objects.h"

#include <cstddef>
#include <span>
#include <vulkan/vulkan_core.h>

namespace ns::vulkan
{
void write_data_to_buffer(
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        VkBuffer buffer,
        VkDeviceSize offset,
        VkDeviceSize size,
        const void* data);

void staging_image_write(
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        VkImage image,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout,
        VkImageAspectFlags aspect_flag,
        VkExtent3D extent,
        std::span<const std::byte> data);

void staging_image_read(
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        VkImage image,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout,
        VkImageAspectFlags aspect_flag,
        VkExtent3D extent,
        std::span<std::byte> data);
}
