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

#pragma once

#include <src/vulkan/buffers.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace ns::gpu::renderer
{
void commands_init_uint32_storage_image(
        VkCommandBuffer command_buffer,
        const vulkan::ImageWithMemory& image,
        std::uint32_t value);

void commands_init_buffer(
        VkCommandBuffer command_buffer,
        const vulkan::BufferWithMemory& src,
        const vulkan::BufferWithMemory& dst);

void commands_read_buffer(
        VkCommandBuffer command_buffer,
        const vulkan::BufferWithMemory& src,
        const vulkan::BufferWithMemory& dst);
}
