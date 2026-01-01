/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "error.h"
#include "objects.h"
#include "queue.h"

#include <vulkan/vulkan_core.h>

#include <functional>
#include <optional>
#include <vector>

namespace ns::vulkan
{
struct CommandBufferCreateInfo final
{
        // required, std::optional is used to check that the values are set
        std::optional<VkDevice> device;
        std::optional<VkRect2D> render_area;
        std::optional<VkRenderPass> render_pass;
        const std::vector<VkFramebuffer>* framebuffers = nullptr;
        std::optional<VkCommandPool> command_pool;

        // optional
        const std::vector<VkClearValue>* clear_values = nullptr;
        std::function<void(VkCommandBuffer command_buffer)> before_render_pass_commands;
        std::function<void(VkCommandBuffer command_buffer)> render_pass_commands;
        std::function<void(VkCommandBuffer command_buffer)> after_render_pass_commands;
};

handle::CommandBuffers create_command_buffers(const CommandBufferCreateInfo& info);

template <typename Commands>
void record_commands(const VkCommandBuffer command_buffer, const Commands& commands)
{
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &info));

        commands();

        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));
}

template <typename Commands>
void run_commands(const VkDevice device, const VkCommandPool pool, const VkQueue queue, const Commands& commands)
{
        const handle::CommandBuffer command_buffer(device, pool);

        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &info));

        commands(command_buffer);

        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));

        queue_submit(command_buffer, queue);
        VULKAN_CHECK(vkQueueWaitIdle(queue));
}
}
