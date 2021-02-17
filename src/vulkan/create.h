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

#include "objects.h"

#include <src/color/color.h>

#include <string>
#include <vector>

namespace ns::vulkan
{
std::vector<Semaphore> create_semaphores(VkDevice device, int count);
std::vector<Fence> create_fences(VkDevice device, int count, bool signaled_state);

PipelineLayout create_pipeline_layout(
        VkDevice device,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);
PipelineLayout create_pipeline_layout(
        VkDevice device,
        const std::vector<unsigned>& set_numbers,
        const std::vector<VkDescriptorSetLayout>& set_layouts);

CommandPool create_command_pool(VkDevice device, uint32_t queue_family_index);
CommandPool create_transient_command_pool(VkDevice device, uint32_t queue_family_index);

Instance create_instance(std::vector<std::string> required_extensions);

Framebuffer create_framebuffer(
        VkDevice device,
        VkRenderPass render_pass,
        uint32_t width,
        uint32_t height,
        const std::vector<VkImageView>& attachments);

VkClearValue color_clear_value(VkFormat format, VkColorSpaceKHR color_space, const Color& color);
VkClearValue depth_stencil_clear_value();
}
