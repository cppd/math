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

#include "transparency.h"

#include "../buffer.h"

namespace ns::gpu::renderer
{
namespace
{
constexpr std::uint32_t HEADS_NULL_INDEX = 0xffff'ffff;

constexpr unsigned long long BUFFER_SIZE = 1ull << 30;

// uint color_rg
// uint color_ba
// uint metalness_roughness
// uint ambient_edge_factor
// float n_x
// float n_y
// float n_z
// float depth
// uint next
constexpr unsigned long long NODE_SIZE = 36;
}

TransparencyBuffers::TransparencyBuffers(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : buffer_size_(std::min<unsigned long long>(
                BUFFER_SIZE,
                device.properties().properties_10.limits.maxStorageBufferRange)),
          node_count_(buffer_size_ / NODE_SIZE),
          node_buffer_(
                  vulkan::BufferMemoryType::DEVICE_LOCAL,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  node_count_ * NODE_SIZE),
          init_buffer_(
                  vulkan::BufferMemoryType::HOST_VISIBLE,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  sizeof(Counters)),
          read_buffer_(
                  vulkan::BufferMemoryType::HOST_VISIBLE,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  sizeof(Counters)),
          counters_(
                  vulkan::BufferMemoryType::DEVICE_LOCAL,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                          | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  sizeof(Counters))
{
        Counters counters;
        counters.transparency_node_counter = 0;
        counters.transparency_overload_counter = 0;
        vulkan::BufferMapper mapper(init_buffer_, 0, init_buffer_.buffer().size());
        mapper.write(counters);
}

unsigned long long TransparencyBuffers::buffer_size() const
{
        return buffer_size_;
}

void TransparencyBuffers::create_buffers(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const VkSampleCountFlagBits sample_count,
        const unsigned width,
        const unsigned height)
{
        delete_buffers();

        heads_.emplace(
                device, family_indices, std::vector<VkFormat>({VK_FORMAT_R32_UINT}), sample_count, VK_IMAGE_TYPE_2D,
                vulkan::make_extent(width, height), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                VK_IMAGE_LAYOUT_GENERAL, command_pool, queue);

        heads_size_.emplace(
                device, family_indices, std::vector<VkFormat>({VK_FORMAT_R32_UINT}), sample_count, VK_IMAGE_TYPE_2D,
                vulkan::make_extent(width, height), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                VK_IMAGE_LAYOUT_GENERAL, command_pool, queue);
}

void TransparencyBuffers::delete_buffers()
{
        heads_.reset();
        heads_size_.reset();
}

const vulkan::Buffer& TransparencyBuffers::counters() const
{
        return counters_.buffer();
}

const vulkan::Buffer& TransparencyBuffers::nodes() const
{
        return node_buffer_.buffer();
}

unsigned TransparencyBuffers::node_count() const
{
        return node_count_;
}

const vulkan::ImageWithMemory& TransparencyBuffers::heads() const
{
        ASSERT(heads_);
        return *heads_;
}

const vulkan::ImageWithMemory& TransparencyBuffers::heads_size() const
{
        ASSERT(heads_size_);
        return *heads_size_;
}

void TransparencyBuffers::commands_init(const VkCommandBuffer command_buffer) const
{
        ASSERT(heads_);
        ASSERT(heads_size_);
        commands_init_uint32_storage_image(command_buffer, *heads_, HEADS_NULL_INDEX);
        commands_init_uint32_storage_image(command_buffer, *heads_size_, 0);
        commands_init_buffer(command_buffer, init_buffer_, counters_);
}

void TransparencyBuffers::commands_read(const VkCommandBuffer command_buffer) const
{
        commands_read_buffer(command_buffer, counters_, read_buffer_);
}

void TransparencyBuffers::read(unsigned long long* const required_node_memory, unsigned* const overload_counter) const
{
        vulkan::BufferMapper mapper(read_buffer_);
        Counters counters;
        mapper.read(&counters);
        *required_node_memory = counters.transparency_node_counter * NODE_SIZE;
        *overload_counter = counters.transparency_overload_counter;
}
}
