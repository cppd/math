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

#include "graphics/vulkan/objects.h"

namespace vulkan_render_implementation
{
vulkan::RenderPass render_pass_swapchain_depth(VkDevice device, VkFormat color_format, VkFormat depth_format);

vulkan::RenderPass render_pass_swapchain(VkDevice device, VkFormat color_format);

vulkan::RenderPass render_pass_swapchain_color_depth(VkDevice device, VkFormat color_format, VkFormat depth_format,
                                                     VkSampleCountFlagBits sample_count);

vulkan::RenderPass render_pass_swapchain_color(VkDevice device, VkFormat color_format, VkSampleCountFlagBits sample_count);

vulkan::RenderPass render_pass_depth(VkDevice device, VkFormat depth_format);

vulkan::RenderPass render_pass_color_depth(VkDevice device, VkFormat color_format, VkFormat depth_format,
                                           VkSampleCountFlagBits sample_count);

vulkan::RenderPass render_pass_color(VkDevice device, VkFormat color_format, VkSampleCountFlagBits sample_count);
}
