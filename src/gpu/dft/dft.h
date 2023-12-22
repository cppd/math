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

#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace ns::gpu::dft
{
class Dft
{
public:
        virtual ~Dft() = default;

        virtual void create_buffers(unsigned width, unsigned height, std::uint32_t family_index) = 0;
        virtual void delete_buffers() = 0;

        virtual void compute_commands(VkCommandBuffer command_buffer, bool inverse) const = 0;

        [[nodiscard]] virtual const vulkan::Buffer& buffer() const = 0;
        [[nodiscard]] virtual const vulkan::BufferWithMemory& buffer_with_memory() const = 0;
};

std::unique_ptr<Dft> create_dft(
        const vulkan::Device* device,
        const vulkan::CommandPool* compute_command_pool,
        const vulkan::Queue* compute_queue,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue,
        vulkan::BufferMemoryType buffer_memory_type,
        const Vector2i& group_size);
}
