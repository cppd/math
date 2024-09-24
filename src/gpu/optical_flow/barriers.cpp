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

#include "barriers.h"

#include <src/com/error.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace ns::gpu::optical_flow
{
void image_barrier(
        const VkCommandBuffer command_buffer,
        const std::vector<VkImage>& images,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        const VkAccessFlags src_access_mask,
        const VkAccessFlags dst_access_mask)
{
        ASSERT(!images.empty());
        ASSERT(command_buffer != VK_NULL_HANDLE);
        ASSERT(std::ranges::all_of(
                images,
                [](const VkImage image)
                {
                        return image != VK_NULL_HANDLE;
                }));

        std::vector<VkImageMemoryBarrier> barriers(images.size());

        for (std::size_t i = 0; i < images.size(); ++i)
        {
                barriers[i] = {};

                barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barriers[i].oldLayout = old_layout;
                barriers[i].newLayout = new_layout;

                barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                barriers[i].image = images[i];

                barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barriers[i].subresourceRange.baseMipLevel = 0;
                barriers[i].subresourceRange.levelCount = 1;
                barriers[i].subresourceRange.baseArrayLayer = 0;
                barriers[i].subresourceRange.layerCount = 1;

                barriers[i].srcAccessMask = src_access_mask;
                barriers[i].dstAccessMask = dst_access_mask;
        }

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
}

void image_barrier(
        const VkCommandBuffer command_buffer,
        const VkImage image,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        const VkAccessFlags src_access_mask,
        const VkAccessFlags dst_access_mask)
{
        image_barrier(command_buffer, std::vector{image}, old_layout, new_layout, src_access_mask, dst_access_mask);
}

void buffer_barrier(
        const VkCommandBuffer command_buffer,
        const VkBuffer buffer,
        const VkPipelineStageFlags dst_stage_mask)
{
        ASSERT(buffer != VK_NULL_HANDLE);

        VkBufferMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer;
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dst_stage_mask, VK_DEPENDENCY_BY_REGION_BIT, 0,
                nullptr, 1, &barrier, 0, nullptr);
}
}
