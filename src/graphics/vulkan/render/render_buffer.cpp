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
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/command.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/print.h"
#include "graphics/vulkan/query.h"

#include <algorithm>
#include <list>
#include <sstream>

namespace
{
std::string buffer_info(const std::vector<vulkan::ColorAttachment>& color, const std::vector<vulkan::DepthAttachment>& depth)
{
        ASSERT(depth.size() > 0);
        ASSERT(std::all_of(color.cbegin(), color.cend(), [&](const vulkan::ColorAttachment& c) {
                return c.sample_count() == color[0].sample_count() && c.format() == color[0].format();
        }));
        ASSERT(std::all_of(depth.cbegin(), depth.cend(),
                           [&](const vulkan::DepthAttachment& d) { return d.format() == depth[0].format(); }));

        std::ostringstream oss;

        oss << "Render buffers sample count = "
            << vulkan::integer_sample_count_flag(color.size() > 0 ? color[0].sample_count() : VK_SAMPLE_COUNT_1_BIT);
        oss << '\n';
        oss << "Render buffers depth attachment format " << vulkan::format_to_string(depth[0].format());

        if (color.size() > 0)
        {
                oss << '\n';
                oss << "Render buffers color attachment format " << vulkan::format_to_string(color[0].format());
        }

        return oss.str();
}

void delete_buffers(std::list<vulkan::CommandBuffers>* command_buffers, std::vector<VkCommandBuffer>* buffers)
{
        ASSERT(command_buffers && buffers);

        if (buffers->size() == 0)
        {
                return;
        }

        // Буферов не предполагается много, поэтому достаточно искать перебором
        for (auto iter = command_buffers->cbegin(); iter != command_buffers->cend(); ++iter)
        {
                if (iter->buffers() == *buffers)
                {
                        command_buffers->erase(iter);
                        buffers->clear();
                        return;
                }
        }

        error_fatal("Command buffers not found");
}

unsigned compute_buffer_count(vulkan::RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain)
{
        switch (buffer_count)
        {
        case vulkan::RenderBufferCount::One:
                return 1;
        case vulkan::RenderBufferCount::Swapchain:
                ASSERT(swapchain.image_views().size() > 0);
                return swapchain.image_views().size();
        }
        error_fatal("Error render buffer count");
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

vulkan::RenderPass create_render_pass_no_depth(VkDevice device, VkFormat swapchain_image_format)
{
        std::array<VkAttachmentDescription, 1> attachments = {};

        // Color
        attachments[0].format = swapchain_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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

vulkan::RenderPass create_multisampling_render_pass_no_depth(VkDevice device, VkSampleCountFlagBits sample_count,
                                                             VkFormat swapchain_image_format)
{
        std::array<VkAttachmentDescription, 2> attachments = {};

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
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference multisampling_color_reference = {};
        multisampling_color_reference.attachment = 1;
        multisampling_color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_resolve_reference = {};
        color_resolve_reference.attachment = 0;
        color_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &multisampling_color_reference;
        subpass_description.pResolveAttachments = &color_resolve_reference;

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

class Impl3D : public vulkan::RenderBuffers3D
{
        virtual std::vector<VkCommandBuffer> create_command_buffers_3d(
                const Color& clear_color,
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) = 0;

        virtual void delete_command_buffers_3d(std::vector<VkCommandBuffer>* buffers) = 0;

        virtual VkPipeline create_pipeline_3d(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                              const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                              const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                              const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) = 0;

        //

        std::vector<VkCommandBuffer> create_command_buffers(
                const Color& clear_color,
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) override final
        {
                return create_command_buffers_3d(clear_color, before_render_pass_commands, commands);
        }

        void delete_command_buffers(std::vector<VkCommandBuffer>* buffers) override final
        {
                delete_command_buffers_3d(buffers);
        }

        VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                   const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override final
        {
                return create_pipeline_3d(primitive_topology, sample_shading, shaders, pipeline_layout, vertex_binding,
                                          vertex_attribute);
        }

protected:
        ~Impl3D() override = default;
};

class Impl2D : public vulkan::RenderBuffers2D
{
        virtual std::vector<VkCommandBuffer> create_command_buffers_2d(
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) = 0;

        virtual void delete_command_buffers_2d(std::vector<VkCommandBuffer>* buffers) = 0;

        virtual VkPipeline create_pipeline_2d(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                              const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                              const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                              const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) = 0;

        //

        std::vector<VkCommandBuffer> create_command_buffers(
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) override final
        {
                return create_command_buffers_2d(before_render_pass_commands, commands);
        }

        void delete_command_buffers(std::vector<VkCommandBuffer>* buffers) override final
        {
                delete_command_buffers_2d(buffers);
        }

        VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                   const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override final
        {
                return create_pipeline_2d(primitive_topology, sample_shading, color_blend, shaders, pipeline_layout,
                                          vertex_binding, vertex_attribute);
        }

protected:
        ~Impl2D() override = default;
};

class Impl final : public vulkan::RenderBuffers, public Impl3D, public Impl2D
{
        const vulkan::Device& m_device;
        VkCommandPool m_graphics_command_pool;
        VkFormat m_swapchain_format;
        VkColorSpaceKHR m_swapchain_color_space;

        //

        std::vector<vulkan::DepthAttachment> m_depth_attachments;
        std::vector<vulkan::ColorAttachment> m_color_attachments;

        vulkan::RenderPass m_render_pass;
        vulkan::RenderPass m_render_pass_no_depth;
        std::vector<vulkan::Framebuffer> m_framebuffers;
        std::vector<vulkan::Framebuffer> m_framebuffers_no_depth;

        std::list<vulkan::CommandBuffers> m_command_buffers;
        std::list<vulkan::CommandBuffers> m_command_buffers_no_depth;
        std::vector<vulkan::Pipeline> m_pipelines;

        void create_multisample(unsigned buffer_count, const vulkan::Swapchain& swapchain, VkSampleCountFlagBits sample_count,
                                const std::vector<uint32_t>& attachment_family_indices, VkQueue graphics_queue,
                                const std::vector<VkFormat>& depth_image_formats);
        void create_one_sample(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                               const std::vector<uint32_t>& attachment_family_indices, VkQueue graphics_queue,
                               const std::vector<VkFormat>& depth_image_formats);

public:
        Impl(vulkan::RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain,
             const std::vector<uint32_t>& attachment_family_indices, const vulkan::Device& device,
             VkCommandPool graphics_command_pool, VkQueue graphics_queue, int required_minimum_sample_count,
             const std::vector<VkFormat>& depth_image_formats);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;

        //

        virtual RenderBuffers3D& buffers_3d() override;
        virtual RenderBuffers2D& buffers_2d() override;

        //

        std::vector<VkCommandBuffer> create_command_buffers_3d(
                const Color& clear_color,
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) override;

        std::vector<VkCommandBuffer> create_command_buffers_2d(
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) override;

        //

        void delete_command_buffers_3d(std::vector<VkCommandBuffer>* buffers) override;

        void delete_command_buffers_2d(std::vector<VkCommandBuffer>* buffers) override;

        //

        VkPipeline create_pipeline_3d(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                      const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                      const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                      const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override;

        VkPipeline create_pipeline_2d(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                      const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                      const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                      const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override;
};

Impl::Impl(vulkan::RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain,
           const std::vector<uint32_t>& attachment_family_indices, const vulkan::Device& device,
           VkCommandPool graphics_command_pool, VkQueue graphics_queue, int required_minimum_sample_count,
           const std::vector<VkFormat>& depth_image_formats)
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

        unsigned count = compute_buffer_count(buffer_count, swapchain);

        if (sample_count != VK_SAMPLE_COUNT_1_BIT)
        {
                create_multisample(count, swapchain, sample_count, attachment_family_indices, graphics_queue,
                                   depth_image_formats);
        }
        else
        {
                create_one_sample(count, swapchain, attachment_family_indices, graphics_queue, depth_image_formats);
        }

        LOG(buffer_info(m_color_attachments, m_depth_attachments));
}

vulkan::RenderBuffers3D& Impl::buffers_3d()
{
        return *this;
}

vulkan::RenderBuffers2D& Impl::buffers_2d()
{
        return *this;
}

void Impl::create_multisample(unsigned buffer_count, const vulkan::Swapchain& swapchain, VkSampleCountFlagBits sample_count,
                              const std::vector<uint32_t>& attachment_family_indices, VkQueue graphics_queue,
                              const std::vector<VkFormat>& depth_image_formats)
{
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_color_attachments.emplace_back(m_device, m_graphics_command_pool, graphics_queue, attachment_family_indices,
                                                 swapchain.format(), sample_count, swapchain.width(), swapchain.height());

                m_depth_attachments.emplace_back(m_device, m_graphics_command_pool, graphics_queue, attachment_family_indices,
                                                 depth_image_formats, sample_count, swapchain.width(), swapchain.height());
        }

        //

        std::vector<VkImageView> attachments;

        m_render_pass =
                create_multisampling_render_pass(m_device, sample_count, swapchain.format(), m_depth_attachments[0].format());

        attachments.resize(3);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] = buffer_count == 1 ? m_color_attachments[0].image_view() : m_color_attachments[i].image_view();
                attachments[2] = buffer_count == 1 ? m_depth_attachments[0].image_view() : m_depth_attachments[i].image_view();

                m_framebuffers.push_back(
                        vulkan::create_framebuffer(m_device, m_render_pass, swapchain.width(), swapchain.height(), attachments));
        }

        //

        m_render_pass_no_depth = create_multisampling_render_pass_no_depth(m_device, sample_count, swapchain.format());

        attachments.resize(2);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] = buffer_count == 1 ? m_color_attachments[0].image_view() : m_color_attachments[i].image_view();

                m_framebuffers_no_depth.push_back(vulkan::create_framebuffer(m_device, m_render_pass_no_depth, swapchain.width(),
                                                                             swapchain.height(), attachments));
        }
}

void Impl::create_one_sample(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                             const std::vector<uint32_t>& attachment_family_indices, VkQueue graphics_queue,
                             const std::vector<VkFormat>& depth_image_formats)
{
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_depth_attachments.emplace_back(m_device, m_graphics_command_pool, graphics_queue, attachment_family_indices,
                                                 depth_image_formats, VK_SAMPLE_COUNT_1_BIT, swapchain.width(),
                                                 swapchain.height());
        }

        //

        std::vector<VkImageView> attachments;

        m_render_pass = create_render_pass(m_device, swapchain.format(), m_depth_attachments[0].format());

        attachments.resize(2);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] = buffer_count == 1 ? m_depth_attachments[0].image_view() : m_depth_attachments[i].image_view();

                m_framebuffers.push_back(
                        create_framebuffer(m_device, m_render_pass, swapchain.width(), swapchain.height(), attachments));
        }

        //

        m_render_pass_no_depth = create_render_pass_no_depth(m_device, swapchain.format());

        attachments.resize(1);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];

                m_framebuffers_no_depth.push_back(
                        create_framebuffer(m_device, m_render_pass_no_depth, swapchain.width(), swapchain.height(), attachments));
        }
}

std::vector<VkCommandBuffer> Impl::create_command_buffers_3d(
        const Color& clear_color,
        const std::optional<std::function<void(VkCommandBuffer command_buffer)>>& before_render_pass_commands,
        const std::function<void(VkCommandBuffer buffer)>& commands)
{
        ASSERT(m_depth_attachments.size() > 0);

        VkClearValue color = vulkan::color_clear_value(m_swapchain_format, m_swapchain_color_space, clear_color);

        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.render_pass = m_render_pass;
        info.framebuffers.emplace(m_framebuffers);
        info.command_pool = m_graphics_command_pool;
        info.before_render_pass_commands = before_render_pass_commands;
        info.render_pass_commands = commands;

        if (m_color_attachments.size() > 0)
        {
                std::array<VkClearValue, 3> clear_values;
                clear_values[0] = color;
                clear_values[1] = color;
                clear_values[2] = vulkan::depth_stencil_clear_value();

                info.clear_values.emplace(clear_values);

                m_command_buffers.push_back(vulkan::create_command_buffers(info));
        }
        else
        {
                std::array<VkClearValue, 2> clear_values;
                clear_values[0] = color;
                clear_values[1] = vulkan::depth_stencil_clear_value();

                info.clear_values.emplace(clear_values);

                m_command_buffers.push_back(vulkan::create_command_buffers(info));
        }

        return m_command_buffers.back().buffers();
}

std::vector<VkCommandBuffer> Impl::create_command_buffers_2d(
        const std::optional<std::function<void(VkCommandBuffer command_buffer)>>& before_render_pass_commands,
        const std::function<void(VkCommandBuffer buffer)>& commands)
{
        ASSERT(m_depth_attachments.size() > 0);

        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.render_pass = m_render_pass_no_depth;
        info.framebuffers.emplace(m_framebuffers_no_depth);
        info.command_pool = m_graphics_command_pool;
        info.before_render_pass_commands = before_render_pass_commands;
        info.render_pass_commands = commands;

        m_command_buffers_no_depth.push_back(vulkan::create_command_buffers(info));

        return m_command_buffers_no_depth.back().buffers();
}

void Impl::delete_command_buffers_3d(std::vector<VkCommandBuffer>* buffers)
{
        delete_buffers(&m_command_buffers, buffers);
}

void Impl::delete_command_buffers_2d(std::vector<VkCommandBuffer>* buffers)
{
        delete_buffers(&m_command_buffers_no_depth, buffers);
}

VkPipeline Impl::create_pipeline_3d(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                    const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                    const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                    const std::vector<VkVertexInputAttributeDescription>& vertex_attribute)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);
        ASSERT(m_depth_attachments.size() > 0);

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = m_render_pass;
        info.sub_pass = 0;
        info.sample_count = m_color_attachments.size() > 0 ? m_color_attachments[0].sample_count() : VK_SAMPLE_COUNT_1_BIT;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.primitive_topology = primitive_topology;
        info.shaders = &shaders;
        info.binding_descriptions = &vertex_binding;
        info.attribute_descriptions = &vertex_attribute;
        info.depth_bias = false;
        info.color_blend = false;

        m_pipelines.push_back(vulkan::create_graphics_pipeline(info));

        return m_pipelines.back();
}

VkPipeline Impl::create_pipeline_2d(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                    const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                    const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                    const std::vector<VkVertexInputAttributeDescription>& vertex_attribute)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);
        ASSERT(m_depth_attachments.size() > 0);

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = m_render_pass_no_depth;
        info.sub_pass = 0;
        info.sample_count = m_color_attachments.size() > 0 ? m_color_attachments[0].sample_count() : VK_SAMPLE_COUNT_1_BIT;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.primitive_topology = primitive_topology;
        info.shaders = &shaders;
        info.binding_descriptions = &vertex_binding;
        info.attribute_descriptions = &vertex_attribute;
        info.depth_bias = false;
        info.color_blend = color_blend;

        m_pipelines.push_back(vulkan::create_graphics_pipeline(info));

        return m_pipelines.back();
}
}

namespace vulkan
{
std::unique_ptr<RenderBuffers> create_render_buffers(RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain,
                                                     const std::vector<uint32_t>& attachment_family_indices,
                                                     const vulkan::Device& device, VkCommandPool graphics_command_pool,
                                                     VkQueue graphics_queue, int required_minimum_sample_count,
                                                     const std::vector<VkFormat>& depth_image_formats)
{
        return std::make_unique<Impl>(buffer_count, swapchain, attachment_family_indices, device, graphics_command_pool,
                                      graphics_queue, required_minimum_sample_count, depth_image_formats);
}
}
