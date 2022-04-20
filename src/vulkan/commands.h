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

#pragma once

#include "objects.h"

#include <functional>
#include <optional>
#include <vector>

namespace ns::vulkan
{
struct CommandBufferCreateInfo final
{
        // required, std::optional is used to check that values are set
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
}
