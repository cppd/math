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

#include <src/image/format.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <span>
#include <vector>

namespace ns::vulkan::buffers
{
void write_pixels_to_image(
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        VkImage image,
        VkFormat format,
        VkExtent3D extent,
        VkImageAspectFlags aspect_flag,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        image::ColorFormat color_format,
        std::span<const std::byte> pixels);

void read_pixels_from_image(
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        VkImage image,
        VkFormat format,
        VkExtent3D extent,
        VkImageAspectFlags aspect_flag,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        image::ColorFormat* color_format,
        std::vector<std::byte>* pixels);
}
