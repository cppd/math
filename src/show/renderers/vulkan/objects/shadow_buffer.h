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
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"
#include "graphics/vulkan/swapchain.h"

#include <functional>
#include <list>
#include <memory>
#include <vector>

namespace vulkan_renderer_implementation
{
class ShadowBuffers
{
        const vulkan::Device& m_device;
        VkCommandPool m_graphics_command_pool;

        //

        std::unique_ptr<vulkan::ShadowDepthAttachment> m_depth_attachment;
        vulkan::RenderPass m_render_pass;
        std::vector<vulkan::Framebuffer> m_framebuffers;
        std::vector<vulkan::Pipeline> m_pipelines;
        std::list<vulkan::CommandBuffers> m_command_buffers;

public:
        ShadowBuffers(const vulkan::Swapchain& swapchain, const std::vector<uint32_t>& attachment_family_indices,
                      const vulkan::Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                      const std::vector<VkFormat>& depth_image_formats, double zoom);

        ShadowBuffers(const ShadowBuffers&) = delete;
        ShadowBuffers& operator=(const ShadowBuffers&) = delete;
        ShadowBuffers& operator=(ShadowBuffers&&) = delete;

        ShadowBuffers(ShadowBuffers&&) = default;
        ~ShadowBuffers() = default;

        //

        const vulkan::ShadowDepthAttachment* texture() const noexcept;

        std::vector<VkCommandBuffer> create_command_buffers(const std::function<void(VkCommandBuffer buffer)>& commands);

        void delete_command_buffers(std::vector<VkCommandBuffer>* buffers);

        VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, const std::vector<const vulkan::Shader*>& shaders,
                                   const vulkan::PipelineLayout& pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions);
};
}
