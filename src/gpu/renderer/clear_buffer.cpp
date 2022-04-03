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

#include "clear_buffer.h"

#include <src/vulkan/commands.h>

namespace ns::gpu::renderer
{
namespace
{
constexpr std::uint32_t IMAGE_CLEAR_VALUE = 0;

void commands_init_storage_image(const VkCommandBuffer command_buffer, const vulkan::ImageWithMemory& image)
{
        ASSERT(image.image().has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image.image().format() == VK_FORMAT_R32_UINT);

        // for vkCmdClearColorImage
        ASSERT(image.image().has_usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT));

        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.image();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        //

        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);

        {
                const VkClearColorValue clear_color = []
                {
                        VkClearColorValue color;
                        color.uint32[0] = IMAGE_CLEAR_VALUE;
                        return color;
                }();

                const VkImageSubresourceRange range = barrier.subresourceRange;

                vkCmdClearColorImage(
                        command_buffer, image.image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);
        }

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);
}
}

ClearBuffer::ClearBuffer(
        const VkDevice device,
        const VkCommandPool graphics_command_pool,
        const RenderBuffers3D* const render_buffers,
        const vulkan::ImageWithMemory* const image,
        const Vector3f& clear_color)
        : device_(device),
          graphics_command_pool_(graphics_command_pool),
          render_buffers_(render_buffers),
          image_(image)
{
        set_color(clear_color);
}

const vulkan::handle::CommandBuffers& ClearBuffer::command_buffer() const
{
        return command_buffers_;
}

void ClearBuffer::set_color(const Vector3f& clear_color)
{
        command_buffers_ = vulkan::handle::CommandBuffers();

        vulkan::CommandBufferCreateInfo info;

        info.device = device_;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = render_buffers_->width();
        info.render_area->extent.height = render_buffers_->height();
        info.render_pass = render_buffers_->render_pass_clear();
        info.framebuffers = &render_buffers_->framebuffers_clear();
        info.command_pool = graphics_command_pool_;

        info.before_render_pass_commands = [&](const VkCommandBuffer command_buffer)
        {
                commands_init_storage_image(command_buffer, *image_);
        };

        const std::vector<VkClearValue> clear_values = render_buffers_->clear_values(clear_color);
        info.clear_values = &clear_values;

        command_buffers_ = vulkan::create_command_buffers(info);
}
}
