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

#include "resolve.h"

#include <src/vulkan/error.h>

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
        unsigned height)
{
        ASSERT(width > 0 && height > 0);
        ASSERT(src_images.size() == dst_images.size());
        ASSERT(!src_images.empty());

        const unsigned buffer_count = src_images.size();

        vulkan::CommandBuffers command_buffers = vulkan::CommandBuffers(device, command_pool, buffer_count);

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkImageResolve image_resolve = {};
        image_resolve.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_resolve.srcSubresource.mipLevel = 0;
        image_resolve.srcSubresource.baseArrayLayer = 0;
        image_resolve.srcSubresource.layerCount = 1;
        image_resolve.srcOffset.x = x;
        image_resolve.srcOffset.y = y;
        image_resolve.srcOffset.z = 0;
        image_resolve.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_resolve.dstSubresource.mipLevel = 0;
        image_resolve.dstSubresource.baseArrayLayer = 0;
        image_resolve.dstSubresource.layerCount = 1;
        image_resolve.dstOffset.x = x;
        image_resolve.dstOffset.y = y;
        image_resolve.dstOffset.z = 0;
        image_resolve.extent.width = width;
        image_resolve.extent.height = height;
        image_resolve.extent.depth = 1;

        VkResult result;

        for (unsigned i = 0; i < buffer_count; ++i)
        {
                const VkCommandBuffer command_buffer = command_buffers[i];

                result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                }

                //

                barrier.image = src_images[i];
                barrier.oldLayout = src_image_layout;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(
                        command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                        nullptr, 0, nullptr, 1, &barrier);

                barrier.image = dst_images[i];
                barrier.oldLayout = dst_image_layout;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                vkCmdPipelineBarrier(
                        command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                        nullptr, 0, nullptr, 1, &barrier);

                //

                vkCmdResolveImage(
                        command_buffer, src_images[i], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_images[i],
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_resolve);

                //

                barrier.image = src_images[i];
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = src_image_layout;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = 0;

                vkCmdPipelineBarrier(
                        command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                        nullptr, 0, nullptr, 1, &barrier);

                barrier.image = dst_images[i];
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = dst_image_layout;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = 0;

                vkCmdPipelineBarrier(
                        command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                        nullptr, 0, nullptr, 1, &barrier);

                //

                result = vkEndCommandBuffer(command_buffer);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                }
        }

        return command_buffers;
}
}
