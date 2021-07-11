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

#include "swapchain.h"

#include "render_pass.h"

#include <src/com/error.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/create.h>
#include <src/vulkan/queue.h>

namespace ns::view
{
Swapchain::Swapchain(
        const VkDevice& device,
        const vulkan::CommandPool& command_pool,
        const RenderBuffers& render_buffers,
        const vulkan::Swapchain& swapchain)
        : m_family_index(command_pool.family_index()),
          m_render_pass(render_pass_swapchain_color(device, swapchain.format(), render_buffers.sample_count()))
{
        const std::vector<VkImageView>& image_views = render_buffers.image_views();

        ASSERT(render_buffers.color_format() == swapchain.format());
        ASSERT(image_views.size() == 1 || swapchain.image_views().size() == image_views.size());

        std::vector<VkImageView> attachments(2);

        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                m_signal_semaphores.emplace_back(device);

                attachments[0] = swapchain.image_views()[i];
                attachments[1] = (image_views.size() == 1) ? image_views[0] : image_views[i];

                m_framebuffers.push_back(vulkan::create_framebuffer(
                        device, m_render_pass, swapchain.width(), swapchain.height(), attachments));
        }

        std::vector<VkFramebuffer> framebuffers;
        framebuffers.reserve(m_framebuffers.size());
        for (const vulkan::Framebuffer& framebuffer : m_framebuffers)
        {
                framebuffers.push_back(framebuffer);
        }

        m_command_buffers = vulkan::CommandBuffers();

        vulkan::CommandBufferCreateInfo info;
        info.device = device;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = swapchain.width();
        info.render_area->extent.height = swapchain.height();
        info.render_pass = m_render_pass;
        info.framebuffers = &framebuffers;
        info.command_pool = command_pool;

        m_command_buffers = vulkan::create_command_buffers(info);
}

VkSemaphore Swapchain::resolve(
        const vulkan::Queue& graphics_queue,
        const VkSemaphore& image_semaphore,
        const VkSemaphore& wait_semaphore,
        const unsigned image_index) const
{
        ASSERT(graphics_queue.family_index() == m_family_index);
        ASSERT(image_index < m_command_buffers.count());
        ASSERT(m_signal_semaphores.size() == 1 || image_index < m_signal_semaphores.size());

        const unsigned semaphore_index = m_signal_semaphores.size() == 1 ? 0 : image_index;

        std::array<VkSemaphore, 2> wait_semaphores;
        std::array<VkPipelineStageFlags, 2> wait_stages;

        wait_semaphores[0] = image_semaphore;
        wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        wait_semaphores[1] = wait_semaphore;
        wait_stages[1] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        vulkan::queue_submit(
                wait_semaphores, wait_stages, m_command_buffers[image_index], m_signal_semaphores[semaphore_index],
                graphics_queue);

        return m_signal_semaphores[semaphore_index];
}
}