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

class TextBuffers
{
        const vulkan::Device& m_device;
        VkCommandPool m_graphics_command_pool;

        //

        unsigned m_width;
        unsigned m_height;
        vulkan::RenderPass m_render_pass;
        std::vector<vulkan::Framebuffer> m_framebuffers;
        std::vector<vulkan::Pipeline> m_pipelines;
        vulkan::CommandBuffers m_command_buffers;

public:
        TextBuffers(const vulkan::Swapchain& swapchain, const std::vector<uint32_t>& attachment_family_indices,
                    const vulkan::Device& device, VkCommandPool graphics_command_pool);

        TextBuffers(const TextBuffers&) = delete;
        TextBuffers& operator=(const TextBuffers&) = delete;
        TextBuffers& operator=(TextBuffers&&) = delete;

        TextBuffers(TextBuffers&&) = default;
        ~TextBuffers() = default;

        //

        void create_command_buffers(const std::function<void(VkCommandBuffer buffer)>& commands);
        void delete_command_buffers();
        const VkCommandBuffer& command_buffer(uint32_t index) const noexcept;

        VkPipeline create_pipeline(const std::vector<const vulkan::Shader*>& shaders,
                                   const vulkan::PipelineLayout& pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions);
};
