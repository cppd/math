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

#include <src/vulkan/buffers.h>

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

        const unsigned node_count_;

        vulkan::ImageWithMemory heads_;
        vulkan::ImageWithMemory heads_size_;
        vulkan::BufferWithMemory node_buffer_;

        vulkan::BufferWithMemory init_buffer_;
        vulkan::BufferWithMemory read_buffer_;
        vulkan::BufferWithMemory counters_;

public:
        TransparencyBuffers(
                const vulkan::Device& device,
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                const std::vector<std::uint32_t>& family_indices,
                VkSampleCountFlagBits sample_count,
                unsigned width,
                unsigned height,
                unsigned long long max_node_buffer_size);

        const vulkan::Buffer& counters() const;
        const vulkan::ImageWithMemory& heads() const;
        const vulkan::ImageWithMemory& heads_size() const;
        const vulkan::Buffer& nodes() const;

        unsigned node_count() const;

        void commands_init(VkCommandBuffer command_buffer) const;
        void commands_read(VkCommandBuffer command_buffer) const;

        void read(unsigned long long* required_node_memory, unsigned* overload_counter) const;
};
}
