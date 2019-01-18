/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/span.h"
#include "graphics/vulkan/objects.h"

#include <array>
#include <functional>
#include <optional>
#include <vector>

namespace vulkan
{
struct CommandBufferCreateInfo
{
        // std::optional для проверки, что значения заданы
        std::optional<VkDevice> device;
        std::optional<uint32_t> width;
        std::optional<uint32_t> height;
        std::optional<VkRenderPass> render_pass;
        std::optional<Span<Framebuffer>> framebuffers;
        std::optional<VkCommandPool> command_pool;
        std::optional<std::function<void(VkCommandBuffer command_buffer)>> render_pass_commands;

        // Эти значения могут быть не заданы
        std::optional<Span<VkClearValue>> clear_values;
        std::optional<std::function<void(VkCommandBuffer command_buffer)>> before_render_pass_commands;
};

CommandBuffers create_command_buffers(const CommandBufferCreateInfo& info);
}
