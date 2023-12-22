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

#pragma once

#include "render_buffers.h"

#include <src/vulkan/objects.h>
#include <src/vulkan/swapchain.h>

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ns::view
{
class Swapchain final
{
        const std::uint32_t family_index_;
        const vulkan::RenderPass render_pass_;
        std::vector<vulkan::handle::Framebuffer> framebuffers_;
        vulkan::handle::CommandBuffers command_buffers_;
        std::vector<vulkan::handle::Semaphore> signal_semaphores_;

public:
        Swapchain(
                VkDevice device,
                const vulkan::CommandPool& command_pool,
                const RenderBuffers& render_buffers,
                const vulkan::Swapchain& swapchain);

        [[nodiscard]] VkSemaphore resolve(
                const vulkan::Queue& graphics_queue,
                VkSemaphore image_semaphore,
                VkSemaphore wait_semaphore,
                unsigned image_index) const;
};
}
