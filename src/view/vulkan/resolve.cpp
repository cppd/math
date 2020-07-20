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

#include <src/com/error.h>

namespace view
{
void commands_resolve(
        VkCommandBuffer command_buffer,
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        const Region<2, int>& rectangle)
{
        ASSERT(rectangle.width() > 0 && rectangle.height() > 0);

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
        image_resolve.srcOffset.x = rectangle.x0();
        image_resolve.srcOffset.y = rectangle.y0();
        image_resolve.srcOffset.z = 0;
        image_resolve.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_resolve.dstSubresource.mipLevel = 0;
        image_resolve.dstSubresource.baseArrayLayer = 0;
        image_resolve.dstSubresource.layerCount = 1;
        image_resolve.dstOffset.x = rectangle.x0();
        image_resolve.dstOffset.y = rectangle.y0();
        image_resolve.dstOffset.z = 0;
        image_resolve.extent.width = rectangle.width();
        image_resolve.extent.height = rectangle.height();
        image_resolve.extent.depth = 1;

        //

        barrier.image = src_image;
        barrier.oldLayout = src_image_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);

        barrier.image = dst_image;
        barrier.oldLayout = dst_image_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);

        //

        vkCmdResolveImage(
                command_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_resolve);

        //

        barrier.image = src_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = src_image_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);

        barrier.image = dst_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = dst_image_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);
}
}
