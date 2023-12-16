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

#include "image_commands.h"

#include <src/com/error.h>
#include <src/numerical/region.h>

namespace ns::view
{
void commands_image_resolve(
        const VkCommandBuffer command_buffer,
        const VkPipelineStageFlags src_stage_before,
        const VkPipelineStageFlags src_stage_after,
        const VkPipelineStageFlags dst_stage_before,
        const VkPipelineStageFlags dst_stage_after,
        const VkAccessFlags src_access_before,
        const VkAccessFlags src_access_after,
        const VkAccessFlags dst_access_before,
        const VkAccessFlags dst_access_after,
        const VkImage src_image,
        const VkImageLayout src_layout,
        const VkImage dst_image,
        const VkImageLayout dst_layout,
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

        //

        barrier.image = src_image;
        barrier.oldLayout = src_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = src_access_before;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
                command_buffer, src_stage_before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        barrier.image = dst_image;
        barrier.oldLayout = dst_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = dst_access_before;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, dst_stage_before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        //

        VkImageResolve resolve = {};
        resolve.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        resolve.srcSubresource.mipLevel = 0;
        resolve.srcSubresource.baseArrayLayer = 0;
        resolve.srcSubresource.layerCount = 1;
        resolve.srcOffset.x = rectangle.x0();
        resolve.srcOffset.y = rectangle.y0();
        resolve.srcOffset.z = 0;
        resolve.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        resolve.dstSubresource.mipLevel = 0;
        resolve.dstSubresource.baseArrayLayer = 0;
        resolve.dstSubresource.layerCount = 1;
        resolve.dstOffset.x = rectangle.x0();
        resolve.dstOffset.y = rectangle.y0();
        resolve.dstOffset.z = 0;
        resolve.extent.width = rectangle.width();
        resolve.extent.height = rectangle.height();
        resolve.extent.depth = 1;

        vkCmdResolveImage(
                command_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolve);

        //

        barrier.image = src_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = src_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = src_access_after;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, src_stage_after, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        barrier.image = dst_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = dst_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = dst_access_after;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage_after, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);
}

void commands_image_copy(
        const VkCommandBuffer command_buffer,
        const VkPipelineStageFlags src_stage_before,
        const VkPipelineStageFlags src_stage_after,
        const VkPipelineStageFlags dst_stage_before,
        const VkPipelineStageFlags dst_stage_after,
        const VkAccessFlags src_access_before,
        const VkAccessFlags src_access_after,
        const VkAccessFlags dst_access_before,
        const VkAccessFlags dst_access_after,
        const VkImageAspectFlags aspect_flags,
        const VkImage src_image,
        const VkImageLayout src_layout,
        const VkImage dst_image,
        const VkImageLayout dst_layout,
        const Region<2, int>& rectangle)
{
        ASSERT(rectangle.width() > 0 && rectangle.height() > 0);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = aspect_flags;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        //

        barrier.image = src_image;
        barrier.oldLayout = src_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = src_access_before;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
                command_buffer, src_stage_before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        barrier.image = dst_image;
        barrier.oldLayout = dst_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = dst_access_before;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, dst_stage_before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        //

        VkImageCopy copy = {};
        copy.srcSubresource.aspectMask = aspect_flags;
        copy.srcSubresource.mipLevel = 0;
        copy.srcSubresource.baseArrayLayer = 0;
        copy.srcSubresource.layerCount = 1;
        copy.srcOffset.x = rectangle.x0();
        copy.srcOffset.y = rectangle.y0();
        copy.srcOffset.z = 0;
        copy.dstSubresource.aspectMask = aspect_flags;
        copy.dstSubresource.mipLevel = 0;
        copy.dstSubresource.baseArrayLayer = 0;
        copy.dstSubresource.layerCount = 1;
        copy.dstOffset.x = rectangle.x0();
        copy.dstOffset.y = rectangle.y0();
        copy.dstOffset.z = 0;
        copy.extent.width = rectangle.width();
        copy.extent.height = rectangle.height();
        copy.extent.depth = 1;

        vkCmdCopyImage(
                command_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        //

        barrier.image = src_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = src_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = src_access_after;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, src_stage_after, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        barrier.image = dst_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = dst_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = dst_access_after;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage_after, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);
}
}
