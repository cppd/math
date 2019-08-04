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

#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"
#include "graphics/vulkan/swapchain.h"

#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

namespace vulkan
{
enum class DepthBufferCount
{
        One,
        Swapchain
};

struct DepthBuffers
{
        virtual ~DepthBuffers() = default;

        virtual const vulkan::DepthAttachmentTexture* texture(unsigned index) const = 0;

        virtual std::vector<VkCommandBuffer> create_command_buffers(
                const std::function<void(VkCommandBuffer buffer)>& commands) = 0;

        virtual void delete_command_buffers(std::vector<VkCommandBuffer>* buffers) = 0;

        virtual VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology,
                                           const std::vector<const vulkan::Shader*>& shaders,
                                           const vulkan::PipelineLayout& pipeline_layout,
                                           const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                           const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) = 0;
};

std::unique_ptr<DepthBuffers> create_depth_buffers(DepthBufferCount buffer_count, const vulkan::Swapchain& swapchain,
                                                   const std::unordered_set<uint32_t>& attachment_family_indices,
                                                   VkCommandPool command_pool, const vulkan::Device& device, double zoom);
}
