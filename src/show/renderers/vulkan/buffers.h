/*
Copyright (C) 2017, 2018 Topological Manifold

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
#include <memory>
#include <string>
#include <vector>

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
        std::vector<vulkan::Framebuffer> m_framebuffers;
        std::vector<vulkan::Pipeline> m_pipelines;
        vulkan::CommandBuffers m_command_buffers;

        std::unique_ptr<vulkan::ShadowDepthAttachment> m_shadow_depth_attachment;
        vulkan::RenderPass m_shadow_render_pass;
        std::vector<vulkan::Framebuffer> m_shadow_framebuffers;
        std::vector<vulkan::Pipeline> m_shadow_pipelines;
        vulkan::CommandBuffers m_shadow_command_buffers;

        std::string main_buffer_info_string() const;
        std::string shadow_buffer_info_string(double zoom, unsigned preferred_width, unsigned preferred_height) const;

        void create_main_buffers(const vulkan::Swapchain& swapchain, const std::vector<uint32_t>& attachment_family_indices,
                                 VkQueue graphics_queue, const std::vector<VkFormat>& depth_image_formats,
                                 VkSampleCountFlagBits sample_count);

        void create_shadow_buffers(unsigned width, unsigned height, const std::vector<uint32_t>& attachment_family_indices,
                                   VkQueue graphics_queue, const std::vector<VkFormat>& depth_image_formats, double shadow_zoom);

public:
        RenderBuffers(const vulkan::Swapchain& swapchain, const std::vector<uint32_t>& attachment_family_indices,
                      const vulkan::Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                      int required_minimum_sample_count, const std::vector<VkFormat>& depth_image_formats, double shadow_zoom);

        RenderBuffers(const RenderBuffers&) = delete;
        RenderBuffers& operator=(const RenderBuffers&) = delete;
        RenderBuffers& operator=(RenderBuffers&&) = delete;

        RenderBuffers(RenderBuffers&&) = default;
        ~RenderBuffers() = default;

        //

        const vulkan::ShadowDepthAttachment* shadow_texture() const noexcept;

        void create_command_buffers(const Color& clear_color, const std::function<void(VkCommandBuffer buffer)>& commands);
        void create_shadow_command_buffers(const std::function<void(VkCommandBuffer buffer)>& shadow_commands);

        void delete_command_buffers();
        void delete_shadow_command_buffers();

        const VkCommandBuffer& command_buffer(uint32_t index) const noexcept;
        const VkCommandBuffer& shadow_command_buffer() const noexcept;

        VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, const std::vector<const vulkan::Shader*>& shaders,
                                   const vulkan::PipelineLayout& pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions);
        VkPipeline create_shadow_pipeline(VkPrimitiveTopology primitive_topology,
                                          const std::vector<const vulkan::Shader*>& shaders,
                                          const vulkan::PipelineLayout& pipeline_layout,
                                          const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                          const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions);

        VkSwapchainKHR swapchain() const noexcept;
        VkFormat swapchain_format() const noexcept;
        VkColorSpaceKHR swapchain_color_space() const noexcept;
};
