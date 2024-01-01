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
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace ns::gpu::renderer
{
class TransparencyBuffers final
{
        struct Counters final
        {
                std::uint32_t transparency_node_counter;
                std::uint32_t transparency_overload_counter;
        };

        const unsigned long long node_size_;
        const unsigned long long buffer_size_;
        const unsigned node_count_;

        const vulkan::BufferWithMemory node_buffer_;

        const vulkan::BufferWithMemory init_buffer_;
        const vulkan::BufferWithMemory read_buffer_;
        const vulkan::BufferWithMemory counters_;

        std::optional<vulkan::ImageWithMemory> heads_;
        std::optional<vulkan::ImageWithMemory> heads_size_;

public:
        TransparencyBuffers(
                bool ray_tracing,
                const vulkan::Device& device,
                const std::vector<std::uint32_t>& family_indices);

        [[nodiscard]] unsigned long long buffer_size() const;

        void create_buffers(
                const vulkan::Device& device,
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                const std::vector<std::uint32_t>& family_indices,
                VkSampleCountFlagBits sample_count,
                unsigned width,
                unsigned height);

        void delete_buffers();

        [[nodiscard]] const vulkan::Buffer& counters() const;
        [[nodiscard]] const vulkan::Buffer& nodes() const;

        [[nodiscard]] unsigned node_count() const;

        [[nodiscard]] const vulkan::ImageWithMemory& heads() const;
        [[nodiscard]] const vulkan::ImageWithMemory& heads_size() const;

        void commands_init(VkCommandBuffer command_buffer) const;
        void commands_read(VkCommandBuffer command_buffer) const;

        struct Info final
        {
                unsigned long long required_node_memory;
                unsigned overload_counter;
        };

        [[nodiscard]] Info read() const;
};
}
