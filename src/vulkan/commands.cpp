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

#include "commands.h"

#include "error.h"

#include <src/com/error.h>

namespace ns::vulkan
{
namespace
{
void begin_command_buffer(const VkCommandBuffer command_buffer)
{
        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_info));
}

void end_command_buffer(const VkCommandBuffer command_buffer)
{
        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));
}
}

handle::CommandBuffers create_command_buffers(const CommandBufferCreateInfo& info)
{
        if (!info.device || !info.render_area || !info.render_pass || !info.framebuffers || !info.command_pool)
        {
                error("No required data to create command buffers");
        }

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = *info.render_pass;
        render_pass_info.renderArea = *info.render_area;
        if (info.clear_values)
        {
                ASSERT(!info.clear_values->empty());
                render_pass_info.clearValueCount = info.clear_values->size();
                render_pass_info.pClearValues = info.clear_values->data();
        }

        handle::CommandBuffers buffers(*info.device, *info.command_pool, info.framebuffers->size());

        for (std::uint32_t i = 0; i < buffers.count(); ++i)
        {
                render_pass_info.framebuffer = (*info.framebuffers)[i];

                begin_command_buffer(buffers[i]);

                if (info.before_render_pass_commands)
                {
                        info.before_render_pass_commands(buffers[i]);
                }

                vkCmdBeginRenderPass(buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

                if (info.render_pass_commands)
                {
                        info.render_pass_commands(buffers[i]);
                }

                vkCmdEndRenderPass(buffers[i]);

                if (info.after_render_pass_commands)
                {
                        info.after_render_pass_commands(buffers[i]);
                }

                end_command_buffer(buffers[i]);
        }

        return buffers;
}
}
