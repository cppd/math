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

#include "shaders/downsample.h"
#include "shaders/flow.h"
#include "shaders/sobel.h"

#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>

#include <array>
#include <vector>

namespace ns::gpu::optical_flow
{
std::vector<vulkan::ImageWithMemory> create_images(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<Vector2i>& sizes,
        VkFormat format,
        uint32_t family_index,
        VkImageUsageFlags usage);

std::vector<vulkan::BufferWithMemory> create_flow_buffers(
        const vulkan::Device& device,
        const std::vector<Vector2i>& sizes,
        uint32_t family_index);

std::vector<DownsampleMemory> create_downsample_memory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images);

std::vector<SobelMemory> create_sobel_memory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
        const std::vector<vulkan::ImageWithMemory>& dx,
        const std::vector<vulkan::ImageWithMemory>& dy);

std::vector<FlowMemory> create_flow_memory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        uint32_t family_index,
        VkSampler sampler,
        const std::vector<Vector2i>& sizes,
        const std::vector<const vulkan::Buffer*>& flow_buffers,
        int top_point_count_x,
        int top_point_count_y,
        const vulkan::Buffer& top_points,
        const vulkan::Buffer& top_flow,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
        const std::vector<vulkan::ImageWithMemory>& dx,
        const std::vector<vulkan::ImageWithMemory>& dy);
}