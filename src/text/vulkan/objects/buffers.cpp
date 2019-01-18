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

#include "buffers.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "graphics/vulkan/command.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/error.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/print.h"
#include "graphics/vulkan/query.h"

namespace
{
vulkan::RenderPass create_render_pass(VkDevice device, VkFormat swapchain_image_format)
{
        std::array<VkAttachmentDescription, 1> attachments = {};

        // Color
        attachments[0].format = swapchain_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_reference = {};
        color_reference.attachment = 0;
        color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_reference;

        std::array<VkSubpassDependency, 1> subpass_dependencies = {};
        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_description;
        create_info.dependencyCount = subpass_dependencies.size();
        create_info.pDependencies = subpass_dependencies.data();

        return vulkan::RenderPass(device, create_info);
}
}

namespace vulkan_text_implementation
{
TextBuffers::TextBuffers(const vulkan::Swapchain& swapchain, const vulkan::Device& device, VkCommandPool graphics_command_pool)
        : m_device(device),
          m_graphics_command_pool(graphics_command_pool),
          m_width(swapchain.width()),
          m_height(swapchain.height())
{
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(graphics_command_pool != VK_NULL_HANDLE);

        m_render_pass = create_render_pass(m_device, swapchain.format());

        std::vector<VkImageView> attachments(1);
        for (VkImageView swapchain_image_view : swapchain.image_views())
        {
                attachments[0] = swapchain_image_view;

                m_framebuffers.push_back(
                        create_framebuffer(m_device, m_render_pass, swapchain.width(), swapchain.height(), attachments));
        }
}

void TextBuffers::create_command_buffers(const std::function<void(VkCommandBuffer buffer)>& commands)
{
        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.width = m_width;
        info.height = m_height;
        info.render_pass = m_render_pass;
        info.framebuffers.emplace(m_framebuffers);
        info.command_pool = m_graphics_command_pool;
        info.render_pass_commands = commands;

        m_command_buffers = vulkan::create_command_buffers(info);
}

VkPipeline TextBuffers::create_pipeline(const std::vector<const vulkan::Shader*>& shaders,
                                        const vulkan::PipelineLayout& pipeline_layout,
                                        const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                        const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = m_render_pass;
        info.sub_pass = 0;
        info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        // info.sample_shading = false;
        info.pipeline_layout = pipeline_layout;
        info.width = m_width;
        info.height = m_height;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.shaders = &shaders;
        info.binding_descriptions = &vertex_binding_descriptions;
        info.attribute_descriptions = &vertex_attribute_descriptions;
        info.depth_bias = false;
        info.color_blend = true;

        m_pipelines.push_back(vulkan::create_graphics_pipeline(info));

        return m_pipelines.back();
}

void TextBuffers::delete_command_buffers()
{
        m_command_buffers = vulkan::CommandBuffers();
}

const VkCommandBuffer& TextBuffers::command_buffer(uint32_t index) const noexcept
{
        return m_command_buffers[index];
}
}
