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

#include "com/color/color.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"
#include "graphics/vulkan/swapchain.h"

#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <vector>

namespace vulkan_renderer_implementation
{
class RenderBuffers
{
        const vulkan::Device& m_device;
        VkCommandPool m_graphics_command_pool;
        VkFormat m_swapchain_format;
        VkColorSpaceKHR m_swapchain_color_space;

        //

        std::unique_ptr<vulkan::DepthAttachment> m_depth_attachment;
        std::unique_ptr<vulkan::ColorAttachment> m_color_attachment;

        vulkan::RenderPass m_render_pass;
        vulkan::RenderPass m_render_pass_no_depth;
        std::vector<vulkan::Framebuffer> m_framebuffers;
        std::vector<vulkan::Framebuffer> m_framebuffers_no_depth;

        std::list<vulkan::CommandBuffers> m_command_buffers;
        std::list<vulkan::CommandBuffers> m_command_buffers_no_depth;
        std::vector<vulkan::Pipeline> m_pipelines;

public:
        RenderBuffers(const vulkan::Swapchain& swapchain, const std::vector<uint32_t>& attachment_family_indices,
                      const vulkan::Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                      int required_minimum_sample_count, const std::vector<VkFormat>& depth_image_formats);

        RenderBuffers(const RenderBuffers&) = delete;
        RenderBuffers& operator=(const RenderBuffers&) = delete;
        RenderBuffers& operator=(RenderBuffers&&) = delete;

        RenderBuffers(RenderBuffers&&) = default;
        ~RenderBuffers() = default;

        //

        std::vector<VkCommandBuffer> create_command_buffers(
                const Color& clear_color, const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass,
                const std::function<void(VkCommandBuffer buffer)>& commands);
        std::vector<VkCommandBuffer> create_command_buffers_no_depth(
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass,
                const std::function<void(VkCommandBuffer buffer)>& commands);
        void delete_command_buffers(std::vector<VkCommandBuffer>* buffers);
        void delete_command_buffers_no_depth(std::vector<VkCommandBuffer>* buffers);

        VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                   const std::vector<const vulkan::Shader*>& shaders,
                                   const vulkan::PipelineLayout& pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions);
        VkPipeline create_pipeline_no_depth(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                            const std::vector<const vulkan::Shader*>& shaders,
                                            const vulkan::PipelineLayout& pipeline_layout,
                                            const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                            const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions);
};
}
