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

#include <src/numerical/vector.h>

#include <string>
#include <vector>

namespace ns::vulkan
{
std::vector<handle::Semaphore> create_semaphores(VkDevice device, int count);

std::vector<handle::Fence> create_fences(VkDevice device, int count, bool signaled_state);

handle::PipelineLayout create_pipeline_layout(
        VkDevice device,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);

handle::PipelineLayout create_pipeline_layout(
        VkDevice device,
        const std::vector<unsigned>& set_numbers,
        const std::vector<VkDescriptorSetLayout>& set_layouts);

CommandPool create_command_pool(VkDevice device, std::uint32_t queue_family_index);

CommandPool create_transient_command_pool(VkDevice device, std::uint32_t queue_family_index);

Instance create_instance(std::vector<std::string> required_extensions);

handle::Framebuffer create_framebuffer(
        VkDevice device,
        VkRenderPass render_pass,
        std::uint32_t width,
        std::uint32_t height,
        const std::vector<VkImageView>& attachments);

VkClearValue create_color_clear_value(VkFormat format, const Vector<3, float>& rgb);

VkClearValue create_depth_stencil_clear_value();

Buffer create_buffer(
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        std::vector<std::uint32_t> family_indices);

Image create_image(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkImageType type,
        VkExtent3D extent,
        VkFormat format,
        std::vector<std::uint32_t> family_indices,
        VkSampleCountFlagBits samples,
        VkImageTiling tiling,
        VkImageUsageFlags usage);

ImageView create_image_view(const Image& image, VkImageAspectFlags aspect_flags);
}
