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

#include "copy.h"

#include "create.h"
#include "memory.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/error.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace ns::vulkan::buffers
{
namespace
{
class StagingBuffer final
{
        Buffer buffer_;
        handle::DeviceMemory memory_;

public:
        StagingBuffer(
                const VkDevice device,
                const VkPhysicalDevice physical_device,
                const std::uint32_t family_index,
                const VkDeviceSize size,
                const VkBufferUsageFlags usage)
                : buffer_(create_buffer(device, size, usage, {family_index})),
                  memory_(create_device_memory(
                          device,
                          physical_device,
                          buffer_.handle(),
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          /*allocate_flags=*/0))
        {
        }

        [[nodiscard]] VkBuffer handle() const&& = delete;

        [[nodiscard]] VkBuffer handle() const&
        {
                return buffer_.handle();
        }

        void write(const VkDeviceSize offset, const VkDeviceSize size, const void* const data) const
        {
                ASSERT(offset + size <= buffer_.size());

                void* pointer = nullptr;
                VULKAN_CHECK(vkMapMemory(memory_.device(), memory_, offset, size, 0, &pointer));
                std::memcpy(pointer, data, size);
                vkUnmapMemory(memory_.device(), memory_);
                // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
        }

        void read(const VkDeviceSize offset, const VkDeviceSize size, void* const data) const
        {
                ASSERT(offset + size <= buffer_.size());

                void* pointer = nullptr;
                VULKAN_CHECK(vkMapMemory(memory_.device(), memory_, offset, size, 0, &pointer));
                std::memcpy(data, pointer, size);
                vkUnmapMemory(memory_.device(), memory_);
                // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
        }
};

VkAccessFlags src_access(const VkImageLayout old_layout)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (old_layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return VK_ACCESS_TRANSFER_READ_BIT;
        default:
                return 0;
        }
#pragma GCC diagnostic pop
}

VkPipelineStageFlags src_stage(const VkImageLayout old_layout)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (old_layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return VK_PIPELINE_STAGE_TRANSFER_BIT;
        default:
                return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
#pragma GCC diagnostic pop
}

VkAccessFlags dst_access(const VkImageLayout new_layout)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (new_layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return VK_ACCESS_TRANSFER_READ_BIT;
        default:
                return 0;
        }
#pragma GCC diagnostic pop
}

VkPipelineStageFlags dst_stage(const VkImageLayout new_layout)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (new_layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return VK_PIPELINE_STAGE_TRANSFER_BIT;
        default:
                return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
#pragma GCC diagnostic pop
}

void cmd_transition_image_layout(
        const VkImageAspectFlags aspect_flags,
        const VkCommandBuffer command_buffer,
        const VkImage image,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout)
{
        if (old_layout == new_layout)
        {
                return;
        }

        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        barrier.subresourceRange.aspectMask = aspect_flags;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = src_access(old_layout);
        barrier.dstAccessMask = dst_access(new_layout);

        vkCmdPipelineBarrier(
                command_buffer, src_stage(old_layout), dst_stage(new_layout), 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void cmd_copy_buffer_to_image(
        const VkImageAspectFlags aspect_flag,
        const VkCommandBuffer command_buffer,
        const VkImage image,
        const VkBuffer buffer,
        const VkExtent3D extent)
{
        VkBufferImageCopy region = {};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = aspect_flag;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = extent;

        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void cmd_copy_image_to_buffer(
        const VkImageAspectFlags aspect_flag,
        const VkCommandBuffer command_buffer,
        const VkBuffer buffer,
        const VkImage image,
        const VkExtent3D extent)
{
        VkBufferImageCopy region = {};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = aspect_flag;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = extent;

        vkCmdCopyImageToBuffer(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
}
}

void write_data_to_buffer(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkBuffer buffer,
        const VkDeviceSize offset,
        const VkDeviceSize size,
        const void* const data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        const StagingBuffer staging_buffer(
                device, physical_device, queue.family_index(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        //

        staging_buffer.write(0, size, data);

        //

        VkBufferCopy copy = {};
        copy.srcOffset = 0;
        copy.dstOffset = offset;
        copy.size = size;

        const auto commands = [&](const VkCommandBuffer command_buffer)
        {
                vkCmdCopyBuffer(command_buffer, staging_buffer.handle(), buffer, 1, &copy);
        };

        run_commands(device, command_pool.handle(), queue.handle(), commands);
}

void staging_image_write(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage image,
        const VkImageLayout old_image_layout,
        const VkImageLayout new_image_layout,
        const VkImageAspectFlags aspect_flag,
        const VkExtent3D extent,
        const std::span<const std::byte> data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        const VkDeviceSize size = data_size(data);

        const StagingBuffer staging_buffer(
                device, physical_device, queue.family_index(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        //

        staging_buffer.write(0, size, data_pointer(data));

        //

        const auto commands = [&](const VkCommandBuffer command_buffer)
        {
                cmd_transition_image_layout(
                        aspect_flag, command_buffer, image, old_image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                cmd_copy_buffer_to_image(aspect_flag, command_buffer, image, staging_buffer.handle(), extent);

                cmd_transition_image_layout(
                        aspect_flag, command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, new_image_layout);
        };

        run_commands(device, command_pool.handle(), queue.handle(), commands);
}

void staging_image_read(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage image,
        const VkImageLayout old_image_layout,
        const VkImageLayout new_image_layout,
        const VkImageAspectFlags aspect_flag,
        const VkExtent3D extent,
        const std::span<std::byte> data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        const VkDeviceSize size = data_size(data);

        const StagingBuffer staging_buffer(
                device, physical_device, queue.family_index(), size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        //

        const auto commands = [&](const VkCommandBuffer command_buffer)
        {
                cmd_transition_image_layout(
                        aspect_flag, command_buffer, image, old_image_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                cmd_copy_image_to_buffer(aspect_flag, command_buffer, staging_buffer.handle(), image, extent);

                cmd_transition_image_layout(
                        aspect_flag, command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, new_image_layout);
        };

        run_commands(device, command_pool.handle(), queue.handle(), commands);

        //

        staging_buffer.read(0, size, data_pointer(data));
}
}
