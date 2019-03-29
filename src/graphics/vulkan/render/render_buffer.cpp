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

#include "render_pass.h"

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
void check_buffers(const std::vector<vulkan::ColorAttachment>& color, const std::vector<vulkan::DepthAttachment>& depth)
{
        if (depth.empty())
        {
                error("No depth attachment");
        }

        if (!std::all_of(color.cbegin(), color.cend(),
                         [&](const vulkan::ColorAttachment& c) { return c.sample_count() == color[0].sample_count(); }))
        {
                error("Color attachments must have the same sample count");
        }

        if (!std::all_of(color.cbegin(), color.cend(),
                         [&](const vulkan::ColorAttachment& c) { return c.format() == color[0].format(); }))
        {
                error("Color attachments must have the same format");
        }

        if (!std::all_of(depth.cbegin(), depth.cend(),
                         [&](const vulkan::DepthAttachment& d) { return d.sample_count() == depth[0].sample_count(); }))
        {
                error("Depth attachments must have the same sample count");
        }

        if (!std::all_of(depth.cbegin(), depth.cend(),
                         [&](const vulkan::DepthAttachment& d) { return d.format() == depth[0].format(); }))
        {
                error("Depth attachments must have the same format");
        }

        if (!std::all_of(color.cbegin(), color.cend(),
                         [&](const vulkan::ColorAttachment& c) { return c.sample_count() == depth[0].sample_count(); }))
        {
                error("Color attachment sample count is not equal to depth attachment sample count");
        }

        if (color.size() == 0)
        {
                if (!std::all_of(depth.cbegin(), depth.cend(),
                                 [&](const vulkan::DepthAttachment& d) { return d.sample_count() == VK_SAMPLE_COUNT_1_BIT; }))
                {
                        error("There are no color attachments, but depth attachment sample count is not equal to 1");
                }
        }
}

std::string buffer_info(const std::vector<vulkan::ColorAttachment>& color, const std::vector<vulkan::DepthAttachment>& depth)
{
        check_buffers(color, depth);

        std::ostringstream oss;

        oss << "Render buffers sample count = "
            << vulkan::integer_sample_count_flag(color.size() > 0 ? color[0].sample_count() : VK_SAMPLE_COUNT_1_BIT);

        oss << '\n';
        if (!depth.empty())
        {
                oss << "Render buffers depth attachment format = " << vulkan::format_to_string(depth[0].format());
        }
        else
        {
                oss << "Render buffers do not have depth attachments";
        }

        oss << '\n';
        if (color.size() > 0)
        {
                oss << "Render buffers color attachment format = " << vulkan::format_to_string(color[0].format());
        }
        else
        {
                oss << "Render buffers do not have color attachments";
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
        const vulkan::VulkanInstance& m_instance;
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
                                const std::vector<uint32_t>& attachment_family_indices,
                                const std::vector<VkFormat>& depth_image_formats);
        void create_one_sample(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                               const std::vector<uint32_t>& attachment_family_indices,
                               const std::vector<VkFormat>& depth_image_formats);

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

public:
        Impl(vulkan::RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain,
             const std::vector<uint32_t>& attachment_family_indices, const vulkan::VulkanInstance& instance,
             int required_minimum_sample_count, const std::vector<VkFormat>& depth_image_formats);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(vulkan::RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain,
           const std::vector<uint32_t>& attachment_family_indices, const vulkan::VulkanInstance& instance,
           int required_minimum_sample_count, const std::vector<VkFormat>& depth_image_formats)
        : m_instance(instance), m_swapchain_format(swapchain.format()), m_swapchain_color_space(swapchain.color_space())
{
        ASSERT(attachment_family_indices.size() > 0);
        ASSERT(depth_image_formats.size() > 0);

        VkSampleCountFlagBits sample_count =
                vulkan::supported_framebuffer_sample_count_flag(m_instance.physical_device(), required_minimum_sample_count);

        unsigned count = compute_buffer_count(buffer_count, swapchain);

        if (sample_count != VK_SAMPLE_COUNT_1_BIT)
        {
                create_multisample(count, swapchain, sample_count, attachment_family_indices, depth_image_formats);
        }
        else
        {
                create_one_sample(count, swapchain, attachment_family_indices, depth_image_formats);
        }

        check_buffers(m_color_attachments, m_depth_attachments);

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
                              const std::vector<uint32_t>& attachment_family_indices,
                              const std::vector<VkFormat>& depth_image_formats)
{
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_color_attachments.emplace_back(m_instance, attachment_family_indices, swapchain.format(), sample_count,
                                                 swapchain.width(), swapchain.height());

                m_depth_attachments.emplace_back(m_instance, attachment_family_indices, depth_image_formats, sample_count,
                                                 swapchain.width(), swapchain.height());
        }

        //

        namespace impl = vulkan_render_implementation;

        std::vector<VkImageView> attachments;

        m_render_pass = impl::render_pass_swapchain_multisample(m_instance.device(), swapchain.format(),
                                                                m_depth_attachments[0].format(), sample_count);

        attachments.resize(3);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] = buffer_count == 1 ? m_color_attachments[0].image_view() : m_color_attachments[i].image_view();
                attachments[2] = buffer_count == 1 ? m_depth_attachments[0].image_view() : m_depth_attachments[i].image_view();

                m_framebuffers.push_back(vulkan::create_framebuffer(m_instance.device(), m_render_pass, swapchain.width(),
                                                                    swapchain.height(), attachments));
        }

        //

        m_render_pass_no_depth =
                impl::render_pass_swapchain_multisample_2d(m_instance.device(), swapchain.format(), sample_count);

        attachments.resize(2);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] = buffer_count == 1 ? m_color_attachments[0].image_view() : m_color_attachments[i].image_view();

                m_framebuffers_no_depth.push_back(vulkan::create_framebuffer(m_instance.device(), m_render_pass_no_depth,
                                                                             swapchain.width(), swapchain.height(), attachments));
        }
}

void Impl::create_one_sample(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                             const std::vector<uint32_t>& attachment_family_indices,
                             const std::vector<VkFormat>& depth_image_formats)
{
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_depth_attachments.emplace_back(m_instance, attachment_family_indices, depth_image_formats,
                                                 VK_SAMPLE_COUNT_1_BIT, swapchain.width(), swapchain.height());
        }

        //

        namespace impl = vulkan_render_implementation;

        std::vector<VkImageView> attachments;

        m_render_pass = impl::render_pass_swapchain(m_instance.device(), swapchain.format(), m_depth_attachments[0].format());

        attachments.resize(2);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] = buffer_count == 1 ? m_depth_attachments[0].image_view() : m_depth_attachments[i].image_view();

                m_framebuffers.push_back(create_framebuffer(m_instance.device(), m_render_pass, swapchain.width(),
                                                            swapchain.height(), attachments));
        }

        //

        m_render_pass_no_depth = impl::render_pass_swapchain_2d(m_instance.device(), swapchain.format());

        attachments.resize(1);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];

                m_framebuffers_no_depth.push_back(create_framebuffer(m_instance.device(), m_render_pass_no_depth,
                                                                     swapchain.width(), swapchain.height(), attachments));
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
        info.device = m_instance.device();
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.render_pass = m_render_pass;
        info.framebuffers.emplace(m_framebuffers);
        info.command_pool = m_instance.graphics_command_pool();
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
        info.device = m_instance.device();
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.render_pass = m_render_pass_no_depth;
        info.framebuffers.emplace(m_framebuffers_no_depth);
        info.command_pool = m_instance.graphics_command_pool();
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

        info.device = &m_instance.device();
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

        info.device = &m_instance.device();
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
                                                     const vulkan::VulkanInstance& instance, int required_minimum_sample_count,
                                                     const std::vector<VkFormat>& depth_image_formats)
{
        return std::make_unique<Impl>(buffer_count, swapchain, attachment_family_indices, instance, required_minimum_sample_count,
                                      depth_image_formats);
}
}
