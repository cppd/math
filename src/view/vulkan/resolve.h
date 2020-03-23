/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/vulkan/buffers.h>
#include <src/vulkan/objects.h>

namespace view_vulkan
{
vulkan::CommandBuffers create_command_buffers_resolve(
        VkDevice device,
        VkCommandPool command_pool,
        const std::vector<VkImage>& src_images,
        VkImageLayout src_image_layout,
        const std::vector<VkImage>& dst_images,
        VkImageLayout dst_image_layout,
        unsigned x,
        unsigned y,
        unsigned width,
        unsigned height);
}
