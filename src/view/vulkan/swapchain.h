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

#pragma once

#include "render_buffers.h"

#include <src/vulkan/swapchain.h>

#include <vector>

namespace ns::view
{
class Swapchain
{
        const uint32_t m_family_index;
        const vulkan::RenderPass m_render_pass;
        std::vector<vulkan::Framebuffer> m_framebuffers;
        vulkan::CommandBuffers m_command_buffers;
        std::vector<vulkan::Semaphore> m_signal_semaphores;

public:
        Swapchain(
                const VkDevice& device,
                const vulkan::CommandPool& command_pool,
                const RenderBuffers& render_buffers,
                const vulkan::Swapchain& swapchain);

        [[nodiscard]] VkSemaphore resolve(
                const vulkan::Queue& graphics_queue,
                const VkSemaphore& image_semaphore,
                const VkSemaphore& wait_semaphore,
                unsigned image_index) const;
};
}
