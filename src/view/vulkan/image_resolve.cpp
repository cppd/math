/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "image_resolve.h"

#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

namespace ns::view
{
namespace
{
void begin_command_buffer(const VkCommandBuffer command_buffer)
{
        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        const VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
        }
}

void end_command_buffer(const VkCommandBuffer command_buffer)
{
        const VkResult result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
        }
}
}

ImageResolve::ImageResolve(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const RenderBuffers& render_buffers,
        const Region<2, int>& rectangle,
        const VkImageLayout image_layout,
        const VkImageUsageFlags usage)
        : family_index_(command_pool.family_index())
{
        const std::size_t count = render_buffers.image_views().size();

        images_.reserve(count);
        signal_semaphores_.reserve(count);

        command_buffers_ = vulkan::CommandBuffers(device, command_pool, count);

        for (unsigned i = 0; i < count; ++i)
        {
                images_.emplace_back(
                        device, std::vector<uint32_t>({command_pool.family_index()}),
                        std::vector<VkFormat>({render_buffers.color_format()}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(render_buffers.width(), render_buffers.height()),
                        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_layout, command_pool, queue);

                signal_semaphores_.emplace_back(device);

                begin_command_buffer(command_buffers_[i]);

                render_buffers.commands_color_resolve(
                        command_buffers_[i], images_[i].image(), image_layout, rectangle, i);

                end_command_buffer(command_buffers_[i]);
        }
}

const vulkan::ImageWithMemory& ImageResolve::image(const unsigned image_index) const
{
        ASSERT(image_index < images_.size());
        return images_[image_index];
}

VkSemaphore ImageResolve::resolve_semaphore(
        const vulkan::Queue& graphics_queue,
        VkSemaphore wait_semaphore,
        const unsigned image_index) const
{
        ASSERT(graphics_queue.family_index() == family_index_);
        ASSERT(image_index < command_buffers_.count());
        ASSERT(image_index < signal_semaphores_.size());

        vulkan::queue_submit(
                wait_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, command_buffers_[image_index],
                signal_semaphores_[image_index], graphics_queue);

        return signal_semaphores_[image_index];
}

void ImageResolve::resolve(const vulkan::Queue& graphics_queue, VkSemaphore wait_semaphore, const unsigned image_index)
        const
{
        ASSERT(graphics_queue.family_index() == family_index_);
        ASSERT(image_index < command_buffers_.count());

        vulkan::queue_submit(
                wait_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, command_buffers_[image_index], graphics_queue);
}
}
