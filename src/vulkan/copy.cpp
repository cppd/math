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

#include "copy.h"

#include <src/com/error.h>

namespace ns::vulkan
{
void commands_image_resolve(
        VkCommandBuffer command_buffer,
        VkPipelineStageFlags src_pipeline_stage_before,
        VkPipelineStageFlags src_pipeline_stage_after,
        VkPipelineStageFlags dst_pipeline_stage_before,
        VkPipelineStageFlags dst_pipeline_stage_after,
        VkAccessFlags src_access_flags_before,
        VkAccessFlags src_access_flags_after,
        VkAccessFlags dst_access_flags_before,
        VkAccessFlags dst_access_flags_after,
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

        //

        barrier.image = src_image;
        barrier.oldLayout = src_image_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = src_access_flags_before;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
                command_buffer, src_pipeline_stage_before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        barrier.image = dst_image;
        barrier.oldLayout = dst_image_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = dst_access_flags_before;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, dst_pipeline_stage_before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        //

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

        vkCmdResolveImage(
                command_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_resolve);

        //

        barrier.image = src_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = src_image_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = src_access_flags_after;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, src_pipeline_stage_after, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

        barrier.image = dst_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = dst_image_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = dst_access_flags_after;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_pipeline_stage_after, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);
}

void commands_image_copy(
        VkCommandBuffer command_buffer,
        VkPipelineStageFlags src_pipeline_stage_before,
        VkPipelineStageFlags src_pipeline_stage_after,
        VkPipelineStageFlags dst_pipeline_stage_before,
        VkPipelineStageFlags dst_pipeline_stage_after,
        VkAccessFlags src_access_flags_before,
        VkAccessFlags src_access_flags_after,
        VkAccessFlags dst_access_flags_before,
        VkAccessFlags dst_access_flags_after,
        VkImageAspectFlags image_aspect_mask,
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        const Region<2, int>& rectangle)
{
        ASSERT(rectangle.width() > 0 && rectangle.height() > 0);

        VkImageMemoryBarrier src_barrier = {};
        src_barrier.image = src_image;
        src_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        src_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        src_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        src_barrier.subresourceRange.aspectMask = image_aspect_mask;
        src_barrier.subresourceRange.baseMipLevel = 0;
        src_barrier.subresourceRange.levelCount = 1;
        src_barrier.subresourceRange.baseArrayLayer = 0;
        src_barrier.subresourceRange.layerCount = 1;

        VkImageMemoryBarrier dst_barrier = {};
        dst_barrier.image = dst_image;
        dst_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        dst_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        dst_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        dst_barrier.subresourceRange.aspectMask = image_aspect_mask;
        dst_barrier.subresourceRange.baseMipLevel = 0;
        dst_barrier.subresourceRange.levelCount = 1;
        dst_barrier.subresourceRange.baseArrayLayer = 0;
        dst_barrier.subresourceRange.layerCount = 1;

        //

        src_barrier.oldLayout = src_image_layout;
        src_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        src_barrier.srcAccessMask = src_access_flags_before;
        src_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
                command_buffer, src_pipeline_stage_before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &src_barrier);

        dst_barrier.oldLayout = dst_image_layout;
        dst_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        dst_barrier.srcAccessMask = dst_access_flags_before;
        dst_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, dst_pipeline_stage_before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &dst_barrier);

        //

        VkImageCopy image_copy = {};
        image_copy.srcSubresource.aspectMask = image_aspect_mask;
        image_copy.srcSubresource.mipLevel = 0;
        image_copy.srcSubresource.baseArrayLayer = 0;
        image_copy.srcSubresource.layerCount = 1;
        image_copy.srcOffset.x = rectangle.x0();
        image_copy.srcOffset.y = rectangle.y0();
        image_copy.srcOffset.z = 0;
        image_copy.dstSubresource.aspectMask = image_aspect_mask;
        image_copy.dstSubresource.mipLevel = 0;
        image_copy.dstSubresource.baseArrayLayer = 0;
        image_copy.dstSubresource.layerCount = 1;
        image_copy.dstOffset.x = rectangle.x0();
        image_copy.dstOffset.y = rectangle.y0();
        image_copy.dstOffset.z = 0;
        image_copy.extent.width = rectangle.width();
        image_copy.extent.height = rectangle.height();
        image_copy.extent.depth = 1;

        vkCmdCopyImage(
                command_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

        //

        src_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        src_barrier.newLayout = src_image_layout;
        src_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        src_barrier.dstAccessMask = src_access_flags_after;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, src_pipeline_stage_after, 0, 0, nullptr, 0, nullptr, 1,
                &src_barrier);

        dst_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        dst_barrier.newLayout = dst_image_layout;
        dst_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_barrier.dstAccessMask = dst_access_flags_after;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_pipeline_stage_after, 0, 0, nullptr, 0, nullptr, 1,
                &dst_barrier);
}
}
