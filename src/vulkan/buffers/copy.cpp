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

#include "copy.h"

#include "create.h"
#include "memory.h"

#include "../create.h"
#include "../error.h"
#include "../memory.h"
#include "../queue.h"

#include <src/com/container.h>

#include <cstring>

namespace ns::vulkan
{
namespace
{
void copy_host_to_device(
        const handle::DeviceMemory& device_memory,
        const VkDeviceSize offset,
        const VkDeviceSize size,
        const void* const data)
{
        void* map_memory_data;

        VULKAN_CHECK(vkMapMemory(device_memory.device(), device_memory, offset, size, 0, &map_memory_data));

        std::memcpy(map_memory_data, data, size);

        vkUnmapMemory(device_memory.device(), device_memory);

        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
}

void copy_device_to_host(
        const handle::DeviceMemory& device_memory,
        const VkDeviceSize offset,
        const VkDeviceSize size,
        void* const data)
{
        void* map_memory_data;

        VULKAN_CHECK(vkMapMemory(device_memory.device(), device_memory, offset, size, 0, &map_memory_data));

        std::memcpy(data, map_memory_data, size);

        vkUnmapMemory(device_memory.device(), device_memory);

        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
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

        VkPipelineStageFlags src_stage;
        VkPipelineStageFlags dst_stage;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (old_layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
        default:
                barrier.srcAccessMask = 0;
                src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (new_layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
        default:
                barrier.dstAccessMask = 0;
                dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
#pragma GCC diagnostic pop

        vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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

void begin_commands(const VkCommandBuffer command_buffer)
{
        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_info));
}

void end_commands(const VkQueue queue, const VkCommandBuffer command_buffer)
{
        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));

        queue_submit(command_buffer, queue);
        VULKAN_CHECK(vkQueueWaitIdle(queue));
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

        Buffer staging_buffer(create_buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {queue.family_index()}));

        handle::DeviceMemory staging_device_memory(create_device_memory(
                device, physical_device, staging_buffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0 /*allocate_flags*/));

        //

        copy_host_to_device(staging_device_memory, 0, size, data);

        handle::CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        VkBufferCopy copy = {};
        copy.srcOffset = 0;
        copy.dstOffset = offset;
        copy.size = size;
        vkCmdCopyBuffer(command_buffer, staging_buffer, buffer, 1, &copy);

        end_commands(queue, command_buffer);
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
        const std::span<const std::byte>& data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        const VkDeviceSize size = data_size(data);

        Buffer staging_buffer(create_buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {queue.family_index()}));

        handle::DeviceMemory staging_device_memory(create_device_memory(
                device, physical_device, staging_buffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0 /*allocate_flags*/));

        //

        copy_host_to_device(staging_device_memory, 0, size, data_pointer(data));

        //

        handle::CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_transition_image_layout(
                aspect_flag, command_buffer, image, old_image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        cmd_copy_buffer_to_image(aspect_flag, command_buffer, image, staging_buffer, extent);

        cmd_transition_image_layout(
                aspect_flag, command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, new_image_layout);

        end_commands(queue, command_buffer);
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
        const std::span<std::byte>& data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        const VkDeviceSize size = data_size(data);

        Buffer staging_buffer(create_buffer(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, {queue.family_index()}));

        handle::DeviceMemory staging_device_memory(create_device_memory(
                device, physical_device, staging_buffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0 /*allocate_flags*/));

        //

        handle::CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_transition_image_layout(
                aspect_flag, command_buffer, image, old_image_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        cmd_copy_image_to_buffer(aspect_flag, command_buffer, staging_buffer, image, extent);

        cmd_transition_image_layout(
                aspect_flag, command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, new_image_layout);

        end_commands(queue, command_buffer);

        //

        copy_device_to_host(staging_device_memory, 0, size, data_pointer(data));
}
}
