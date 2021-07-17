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

#include "image.h"

#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

namespace ns::view
{
Image::Image(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const RenderBuffers& render_buffers,
        const Region<2, int>& rectangle,
        const VkImageLayout image_layout,
        const VkImageUsageFlags usage)
        : m_family_index(command_pool.family_index())
{
        const std::size_t count = render_buffers.image_views().size();

        m_images.reserve(count);
        m_signal_semaphores.reserve(count);

        m_command_buffers = vulkan::CommandBuffers(device, command_pool, count);

        for (unsigned i = 0; i < count; ++i)
        {
                m_images.emplace_back(
                        device, std::vector<uint32_t>({command_pool.family_index()}),
                        std::vector<VkFormat>({render_buffers.color_format()}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(render_buffers.width(), render_buffers.height()),
                        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_layout, command_pool, queue);

                m_signal_semaphores.emplace_back(device);

                VkCommandBufferBeginInfo command_buffer_info = {};
                command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

                VkResult result;

                result = vkBeginCommandBuffer(m_command_buffers[i], &command_buffer_info);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                }

                render_buffers.commands_color_resolve(
                        m_command_buffers[i], m_images[i].image(), image_layout, rectangle, i);

                result = vkEndCommandBuffer(m_command_buffers[i]);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                }
        }
}

const vulkan::ImageWithMemory& Image::image(const unsigned image_index) const
{
        ASSERT(image_index < m_images.size());
        return m_images[image_index];
}

VkSemaphore Image::resolve(const vulkan::Queue& graphics_queue, VkSemaphore wait_semaphore, const unsigned image_index)
        const
{
        ASSERT(graphics_queue.family_index() == m_family_index);
        ASSERT(image_index < m_command_buffers.count());
        ASSERT(image_index < m_signal_semaphores.size());

        vulkan::queue_submit(
                wait_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_command_buffers[image_index],
                m_signal_semaphores[image_index], graphics_queue);

        return m_signal_semaphores[image_index];
}

void Image::resolve(const vulkan::Queue& graphics_queue, const unsigned image_index) const
{
        ASSERT(graphics_queue.family_index() == m_family_index);
        ASSERT(image_index < m_command_buffers.count());

        vulkan::queue_submit(m_command_buffers[image_index], graphics_queue);
}
}
