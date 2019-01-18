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

#include "render_buffer.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "graphics/vulkan/command.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/error.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/print.h"
#include "graphics/vulkan/query.h"

#include <sstream>

namespace
{
std::string buffer_info(const vulkan::ColorAttachment* color, const vulkan::DepthAttachment* depth)
{
        std::ostringstream oss;

        oss << "Main buffers sample count = "
            << vulkan::integer_sample_count_flag(color ? color->sample_count() : VK_SAMPLE_COUNT_1_BIT);
        oss << '\n';
        oss << "Main buffers depth attachment format " << vulkan::format_to_string(depth->format());

        if (color)
        {
                oss << '\n';
                oss << "Main buffers color attachment format " << vulkan::format_to_string(color->format());
        }

        return oss.str();
}

vulkan::RenderPass create_render_pass(VkDevice device, VkFormat swapchain_image_format, VkFormat depth_image_format)
{
        std::array<VkAttachmentDescription, 2> attachments = {};

        // Color
        attachments[0].format = swapchain_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Depth
        attachments[1].format = depth_image_format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_reference = {};
        color_reference.attachment = 0;
        color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 1;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_reference;
        subpass_description.pDepthStencilAttachment = &depth_reference;

#if 1
        std::array<VkSubpassDependency, 1> subpass_dependencies = {};
        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
#else
        std::array<VkSubpassDependency, 2> subpass_dependencies = {};

        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpass_dependencies[1].srcSubpass = 0;
        subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[1].dstAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
#endif

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

vulkan::RenderPass create_multisampling_render_pass(VkDevice device, VkSampleCountFlagBits sample_count,
                                                    VkFormat swapchain_image_format, VkFormat depth_image_format)
{
        std::array<VkAttachmentDescription, 3> attachments = {};

        // Color resolve
        attachments[0].format = swapchain_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Multisampling color
        attachments[1].format = swapchain_image_format;
        attachments[1].samples = sample_count;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Multisampling depth
        attachments[2].format = depth_image_format;
        attachments[2].samples = sample_count;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference multisampling_color_reference = {};
        multisampling_color_reference.attachment = 1;
        multisampling_color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference multisampling_depth_reference = {};
        multisampling_depth_reference.attachment = 2;
        multisampling_depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_resolve_reference = {};
        color_resolve_reference.attachment = 0;
        color_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &multisampling_color_reference;
        subpass_description.pResolveAttachments = &color_resolve_reference;
        subpass_description.pDepthStencilAttachment = &multisampling_depth_reference;

#if 1
        std::array<VkSubpassDependency, 1> subpass_dependencies = {};
        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
#else
        std::array<VkSubpassDependency, 2> subpass_dependencies = {};

        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpass_dependencies[1].srcSubpass = 0;
        subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[1].dstAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
#endif

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

namespace vulkan_renderer_implementation
{
MainBuffers::MainBuffers(const vulkan::Swapchain& swapchain, const std::vector<uint32_t>& attachment_family_indices,
                         const vulkan::Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                         int required_minimum_sample_count, const std::vector<VkFormat>& depth_image_formats)
        : m_device(device),
          m_graphics_command_pool(graphics_command_pool),
          m_swapchain_format(swapchain.format()),
          m_swapchain_color_space(swapchain.color_space())
{
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(graphics_command_pool != VK_NULL_HANDLE);
        ASSERT(graphics_queue != VK_NULL_HANDLE);
        ASSERT(attachment_family_indices.size() > 0);
        ASSERT(depth_image_formats.size() > 0);

        VkSampleCountFlagBits sample_count =
                vulkan::supported_framebuffer_sample_count_flag(device.physical_device(), required_minimum_sample_count);

        if (sample_count != VK_SAMPLE_COUNT_1_BIT)
        {
                m_color_attachment = std::make_unique<vulkan::ColorAttachment>(
                        m_device, m_graphics_command_pool, graphics_queue, attachment_family_indices, swapchain.format(),
                        sample_count, swapchain.width(), swapchain.height());

                m_depth_attachment = std::make_unique<vulkan::DepthAttachment>(
                        m_device, m_graphics_command_pool, graphics_queue, attachment_family_indices, depth_image_formats,
                        sample_count, swapchain.width(), swapchain.height());

                m_render_pass = create_multisampling_render_pass(m_device, sample_count, swapchain.format(),
                                                                 m_depth_attachment->format());

                std::vector<VkImageView> attachments(3);
                for (VkImageView swapchain_image_view : swapchain.image_views())
                {
                        attachments[0] = swapchain_image_view;
                        attachments[1] = m_color_attachment->image_view();
                        attachments[2] = m_depth_attachment->image_view();

                        m_framebuffers.push_back(vulkan::create_framebuffer(m_device, m_render_pass, swapchain.width(),
                                                                            swapchain.height(), attachments));
                }
        }
        else
        {
                m_depth_attachment = std::make_unique<vulkan::DepthAttachment>(
                        m_device, m_graphics_command_pool, graphics_queue, attachment_family_indices, depth_image_formats,
                        VK_SAMPLE_COUNT_1_BIT, swapchain.width(), swapchain.height());

                m_render_pass = create_render_pass(m_device, swapchain.format(), m_depth_attachment->format());

                std::vector<VkImageView> attachments(2);
                for (VkImageView swapchain_image_view : swapchain.image_views())
                {
                        attachments[0] = swapchain_image_view;
                        attachments[1] = m_depth_attachment->image_view();

                        m_framebuffers.push_back(
                                create_framebuffer(m_device, m_render_pass, swapchain.width(), swapchain.height(), attachments));
                }
        }

        LOG(buffer_info(m_color_attachment.get(), m_depth_attachment.get()));
}

void MainBuffers::create_command_buffers(
        const Color& clear_color, const std::optional<std::function<void(VkCommandBuffer command_buffer)>>& before_render_pass,
        const std::function<void(VkCommandBuffer buffer)>& commands)
{
        VkClearValue color = vulkan::color_clear_value(m_swapchain_format, m_swapchain_color_space, clear_color);

        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.width = m_depth_attachment->width();
        info.height = m_depth_attachment->height();
        info.render_pass = m_render_pass;
        info.framebuffers.emplace(m_framebuffers);
        info.command_pool = m_graphics_command_pool;
        info.before_render_pass_commands = before_render_pass;
        info.render_pass_commands = commands;

        if (m_color_attachment)
        {
                std::array<VkClearValue, 3> clear_values;
                clear_values[0] = color;
                clear_values[1] = color;
                clear_values[2] = vulkan::depth_stencil_clear_value();

                info.clear_values.emplace(clear_values);

                m_command_buffers = vulkan::create_command_buffers(info);
        }
        else
        {
                std::array<VkClearValue, 2> clear_values;
                clear_values[0] = color;
                clear_values[1] = vulkan::depth_stencil_clear_value();

                info.clear_values.emplace(clear_values);

                m_command_buffers = vulkan::create_command_buffers(info);
        }
}

VkPipeline MainBuffers::create_pipeline(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                        const std::vector<const vulkan::Shader*>& shaders,
                                        const vulkan::PipelineLayout& pipeline_layout,
                                        const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                        const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = m_render_pass;
        info.sub_pass = 0;
        info.sample_count = m_color_attachment ? m_color_attachment->sample_count() : VK_SAMPLE_COUNT_1_BIT;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout;
        info.width = m_depth_attachment->width();
        info.height = m_depth_attachment->height();
        info.primitive_topology = primitive_topology;
        info.shaders = &shaders;
        info.binding_descriptions = &vertex_binding_descriptions;
        info.attribute_descriptions = &vertex_attribute_descriptions;
        info.depth_bias = false;
        info.color_blend = false;

        m_pipelines.push_back(vulkan::create_graphics_pipeline(info));

        return m_pipelines.back();
}

void MainBuffers::delete_command_buffers()
{
        m_command_buffers = vulkan::CommandBuffers();
}

const VkCommandBuffer& MainBuffers::command_buffer(uint32_t index) const noexcept
{
        return m_command_buffers[index];
}
}
