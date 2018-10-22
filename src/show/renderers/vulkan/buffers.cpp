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

#include "buffers.h"

#include "pipeline.h"

#include "com/error.h"
#include "com/log.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/error.h"
#include "graphics/vulkan/print.h"
#include "graphics/vulkan/query.h"

namespace
{
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

vulkan::RenderPass create_shadow_render_pass(VkDevice device, VkFormat depth_image_format)
{
        std::array<VkAttachmentDescription, 1> attachments = {};

        // Depth
        attachments[0].format = depth_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 0;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 0;
        subpass_description.pDepthStencilAttachment = &depth_reference;

        std::array<VkSubpassDependency, 2> subpass_dependencies = {};

        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpass_dependencies[0].srcAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[0].dstAccessMask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpass_dependencies[1].srcSubpass = 0;
        subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        subpass_dependencies[1].srcAccessMask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

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

template <size_t N>
vulkan::CommandBuffers create_command_buffers(VkDevice device, uint32_t width, uint32_t height, VkRenderPass render_pass,
                                              const std::vector<vulkan::Framebuffer>& framebuffers, VkCommandPool command_pool,
                                              const std::array<VkClearValue, N>& clear_values,
                                              const std::function<void(VkCommandBuffer command_buffer)>& commands)
{
        VkResult result;

        vulkan::CommandBuffers command_buffers(device, command_pool, framebuffers.size());

        for (uint32_t i = 0; i < command_buffers.count(); ++i)
        {
                VkCommandBufferBeginInfo command_buffer_info = {};
                command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                // command_buffer_info.pInheritanceInfo = nullptr;

                result = vkBeginCommandBuffer(command_buffers[i], &command_buffer_info);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                }

                VkRenderPassBeginInfo render_pass_info = {};
                render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_info.renderPass = render_pass;
                render_pass_info.framebuffer = framebuffers[i];
                render_pass_info.renderArea.offset.x = 0;
                render_pass_info.renderArea.offset.y = 0;
                render_pass_info.renderArea.extent.width = width;
                render_pass_info.renderArea.extent.height = height;
                render_pass_info.clearValueCount = clear_values.size();
                render_pass_info.pClearValues = clear_values.data();

                vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

#if 1
                commands(command_buffers[i]);

#else
                vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                                        &descriptor_set, 0, nullptr);

                VkBuffer vertex_buffers[] = {vertex_buffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);

#if 0
                vkCmdDraw(command_buffers[i], vertex_count, 1, 0, 0);
#else
                vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, index_type);
                vkCmdDrawIndexed(command_buffers[i], vertex_count, 1, 0, 0, 0);
#endif
#endif

                vkCmdEndRenderPass(command_buffers[i]);

                result = vkEndCommandBuffer(command_buffers[i]);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                }
        }

        return command_buffers;
}
}

RenderBuffers::RenderBuffers(const vulkan::Swapchain& swapchain, const std::vector<uint32_t>& attachment_family_indices,
                             const vulkan::Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                             int required_minimum_sample_count, const std::vector<VkFormat>& depth_image_formats,
                             double shadow_zoom)
        : m_device(device),
          m_graphics_command_pool(graphics_command_pool),
          m_swapchain_format(swapchain.format()),
          m_swapchain_color_space(swapchain.color_space())
{
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(graphics_command_pool != VK_NULL_HANDLE);

        VkSampleCountFlagBits sample_count =
                vulkan::supported_framebuffer_sample_count_flag(device.physical_device(), required_minimum_sample_count);

        create_main_buffers(swapchain, attachment_family_indices, graphics_queue, depth_image_formats, sample_count);

        create_shadow_buffers(swapchain.width(), swapchain.height(), attachment_family_indices, graphics_queue,
                              depth_image_formats, shadow_zoom);
}

void RenderBuffers::create_main_buffers(const vulkan::Swapchain& swapchain,
                                        const std::vector<uint32_t>& attachment_family_indices, VkQueue graphics_queue,
                                        const std::vector<VkFormat>& depth_image_formats, VkSampleCountFlagBits sample_count)
{
        ASSERT(graphics_queue != VK_NULL_HANDLE);
        ASSERT(attachment_family_indices.size() > 0);
        ASSERT(depth_image_formats.size() > 0);

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

        LOG(main_buffer_info_string());
}

void RenderBuffers::create_shadow_buffers(unsigned width, unsigned height, const std::vector<uint32_t>& attachment_family_indices,
                                          VkQueue graphics_queue, const std::vector<VkFormat>& depth_image_formats,
                                          double shadow_zoom)
{
        ASSERT(graphics_queue != VK_NULL_HANDLE);
        ASSERT(attachment_family_indices.size() > 0);
        ASSERT(depth_image_formats.size() > 0);

        shadow_zoom = std::max(shadow_zoom, 1.0);

        unsigned preferred_width = std::lround(width * shadow_zoom);
        unsigned preferred_height = std::lround(height * shadow_zoom);

        m_shadow_depth_attachment = std::make_unique<vulkan::ShadowDepthAttachment>(
                m_device, m_graphics_command_pool, graphics_queue, attachment_family_indices, depth_image_formats,
                preferred_width, preferred_height);

        m_shadow_render_pass = create_shadow_render_pass(m_device, m_shadow_depth_attachment->format());

        std::vector<VkImageView> shadow_attachments(1);
        shadow_attachments[0] = m_shadow_depth_attachment->image_view();

        m_shadow_framebuffers.push_back(create_framebuffer(m_device, m_shadow_render_pass, m_shadow_depth_attachment->width(),
                                                           m_shadow_depth_attachment->height(), shadow_attachments));

        LOG(shadow_buffer_info_string(shadow_zoom, preferred_width, preferred_height));
}

void RenderBuffers::create_command_buffers(const Color& clear_color, const std::function<void(VkCommandBuffer buffer)>& commands)
{
        VkClearValue color = vulkan::color_clear_value(m_swapchain_format, m_swapchain_color_space, clear_color);

        if (m_color_attachment->sample_count() != VK_SAMPLE_COUNT_1_BIT)
        {
                std::array<VkClearValue, 3> clear_values;
                clear_values[0] = color;
                clear_values[1] = color;
                clear_values[2] = vulkan::depth_stencil_clear_value();

                m_command_buffers =
                        ::create_command_buffers(m_device, m_depth_attachment->width(), m_depth_attachment->height(),
                                                 m_render_pass, m_framebuffers, m_graphics_command_pool, clear_values, commands);
        }
        else
        {
                std::array<VkClearValue, 2> clear_values;
                clear_values[0] = color;
                clear_values[1] = vulkan::depth_stencil_clear_value();

                m_command_buffers =
                        ::create_command_buffers(m_device, m_depth_attachment->width(), m_depth_attachment->height(),
                                                 m_render_pass, m_framebuffers, m_graphics_command_pool, clear_values, commands);
        }
}

void RenderBuffers::create_shadow_command_buffers(const std::function<void(VkCommandBuffer buffer)>& shadow_commands)
{
        std::array<VkClearValue, 1> clear_values;
        clear_values[0] = vulkan::depth_stencil_clear_value();

        m_shadow_command_buffers = ::create_command_buffers(
                m_device, m_shadow_depth_attachment->width(), m_shadow_depth_attachment->height(), m_shadow_render_pass,
                m_shadow_framebuffers, m_graphics_command_pool, clear_values, shadow_commands);
}

std::string RenderBuffers::main_buffer_info_string() const
{
        std::string s;
        s += "Sample count = " + to_string(vulkan::integer_sample_count_flag(m_color_attachment->sample_count()));
        s += '\n';
        s += "Depth attachment format " + vulkan::format_to_string(m_depth_attachment->format());
        if (m_color_attachment)
        {
                s += '\n';
                s += "Color attachment format " + vulkan::format_to_string(m_color_attachment->format());
        }
        return s;
}

std::string RenderBuffers::shadow_buffer_info_string(double zoom, unsigned preferred_width, unsigned preferred_height) const
{
        std::string s;
        s += "Shadow depth attachment format " + vulkan::format_to_string(m_shadow_depth_attachment->format());
        s += '\n';
        s += "Shadow zoom " + to_string_fixed(zoom, 5);
        s += '\n';
        s += "Requested shadow size (" + to_string(preferred_width) + ", " + to_string(preferred_height) + ")";
        s += '\n';
        s += "Chosen shadow size (" + to_string(m_shadow_depth_attachment->width()) + ", " +
             to_string(m_shadow_depth_attachment->height()) + ")";
        return s;
}

const vulkan::ShadowDepthAttachment* RenderBuffers::shadow_texture() const noexcept
{
        return m_shadow_depth_attachment.get();
}

VkPipeline RenderBuffers::create_pipeline(VkPrimitiveTopology primitive_topology,
                                          const std::vector<const vulkan::Shader*>& shaders,
                                          const vulkan::PipelineLayout& pipeline_layout,
                                          const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                          const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);

        m_pipelines.push_back(create_graphics_pipeline(
                m_device, m_render_pass, 0 /*sub_pass*/, m_color_attachment->sample_count(), pipeline_layout,
                m_depth_attachment->width(), m_depth_attachment->height(), primitive_topology, shaders,
                vertex_binding_descriptions, vertex_attribute_descriptions));

        return m_pipelines.back();
}

VkPipeline RenderBuffers::create_shadow_pipeline(
        VkPrimitiveTopology primitive_topology, const std::vector<const vulkan::Shader*>& shaders,
        const vulkan::PipelineLayout& pipeline_layout,
        const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
        const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);

        m_shadow_pipelines.push_back(create_shadow_graphics_pipeline(
                m_device, m_shadow_render_pass, 0 /*sub_pass*/, VK_SAMPLE_COUNT_1_BIT, pipeline_layout,
                m_shadow_depth_attachment->width(), m_shadow_depth_attachment->height(), primitive_topology, shaders,
                vertex_binding_descriptions, vertex_attribute_descriptions));

        return m_shadow_pipelines.back();
}

void RenderBuffers::delete_command_buffers()
{
        m_command_buffers = vulkan::CommandBuffers();
}

void RenderBuffers::delete_shadow_command_buffers()
{
        m_shadow_command_buffers = vulkan::CommandBuffers();
}

const VkCommandBuffer& RenderBuffers::command_buffer(uint32_t index) const noexcept
{
        return m_command_buffers[index];
}

const VkCommandBuffer& RenderBuffers::shadow_command_buffer() const noexcept
{
        return m_shadow_command_buffers[0];
}

//
