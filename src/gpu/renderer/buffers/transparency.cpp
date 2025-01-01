/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "commands.h"

#include <src/com/error.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
constexpr std::uint32_t HEADS_NULL_INDEX = 0xffff'ffff;

constexpr unsigned long long BUFFER_SIZE = 1ull << 31;

// uint color_rgba;
// uint metalness_roughness_ambient_edge_factor;
// float n_x;
// float n_y;
// float n_z;
// float depth;
// #ifdef RAY_TRACING
//  float world_position_x;
//  float world_position_y;
//  float world_position_z;
//  float geometric_normal_x;
//  float geometric_normal_y;
//  float geometric_normal_z;
// #endif
// uint next;
unsigned node_size(const bool ray_tracing)
{
        if (ray_tracing)
        {
                return 13 * 4;
        }
        return 7 * 4;
}
}

TransparencyBuffers::TransparencyBuffers(
        const bool ray_tracing,
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices)
        : node_size_(node_size(ray_tracing)),
          buffer_size_(std::min<unsigned long long>(
                  BUFFER_SIZE,
                  device.properties().properties_10.limits.maxStorageBufferRange)),
          node_count_(buffer_size_ / node_size_),
          node_buffer_(
                  vulkan::BufferMemoryType::DEVICE_LOCAL,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  node_count_ * node_size_),
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
        const vulkan::BufferMapper mapper(init_buffer_, 0, init_buffer_.buffer().size());
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
                device, family_indices, std::vector({VK_FORMAT_R32_UINT}), sample_count, VK_IMAGE_TYPE_2D,
                vulkan::make_extent(width, height), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                VK_IMAGE_LAYOUT_GENERAL, command_pool, queue);

        heads_size_.emplace(
                device, family_indices, std::vector({VK_FORMAT_R32_UINT}), sample_count, VK_IMAGE_TYPE_2D,
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

TransparencyBuffers::Info TransparencyBuffers::read() const
{
        const vulkan::BufferMapper mapper(read_buffer_);
        Counters counters;
        mapper.read(&counters);
        return {.required_node_memory = counters.transparency_node_counter * node_size_,
                .overload_counter = counters.transparency_overload_counter};
}
}
