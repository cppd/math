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

#include "depth_buffer.h"

#include "render_pass.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "graphics/vulkan/commands.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/print.h"

#include <algorithm>
#include <list>
#include <sstream>

// clang-format off
constexpr std::initializer_list<VkFormat> DEPTH_IMAGE_FORMATS =
{
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
};
// clang-format on

namespace
{
void check_buffers(const std::vector<vulkan::DepthAttachmentTexture>& depth)
{
        if (depth.empty())
        {
                error("No depth attachment");
        }

        if (!std::all_of(depth.cbegin(), depth.cend(),
                         [&](const vulkan::DepthAttachmentTexture& d) { return d.format() == depth[0].format(); }))
        {
                error("Depth attachments must have the same format");
        }

        if (!std::all_of(depth.cbegin(), depth.cend(), [&](const vulkan::DepthAttachmentTexture& d) {
                    return d.width() == depth[0].width() && d.height() == depth[0].height();
            }))
        {
                error("Depth attachments must have the same size");
        }
}

std::string buffer_info(const std::vector<vulkan::DepthAttachmentTexture>& depth, double zoom, unsigned width, unsigned height)
{
        check_buffers(depth);

        std::ostringstream oss;

        oss << "Depth buffers format " << vulkan::format_to_string(depth[0].format());
        oss << '\n';
        oss << "Depth buffers zoom = " << to_string_fixed(zoom, 5);
        oss << '\n';
        oss << "Depth buffers requested size = (" << width << ", " << height << ")";
        oss << '\n';
        oss << "Depth buffers chosen size = (" << depth[0].width() << ", " << depth[0].height() << ")";

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

        error_fatal("Depth command buffers not found");
}

unsigned compute_buffer_count(vulkan::DepthBufferCount buffer_count, const vulkan::Swapchain& swapchain)
{
        switch (buffer_count)
        {
        case vulkan::DepthBufferCount::One:
                return 1;
        case vulkan::DepthBufferCount::Swapchain:
                ASSERT(swapchain.image_views().size() > 0);
                return swapchain.image_views().size();
        }
        error_fatal("Error depth buffer count");
}

class Impl final : public vulkan::DepthBuffers
{
        const vulkan::Device& m_device;
        VkCommandPool m_command_pool;

        //

        std::vector<vulkan::DepthAttachmentTexture> m_depth_attachments;
        vulkan::RenderPass m_render_pass;
        std::vector<vulkan::Framebuffer> m_framebuffers;

        std::list<vulkan::CommandBuffers> m_command_buffers;
        std::vector<vulkan::Pipeline> m_pipelines;

        //

        const vulkan::DepthAttachmentTexture* texture(unsigned index) const override;

        std::vector<VkCommandBuffer> create_command_buffers(const std::function<void(VkCommandBuffer buffer)>& commands) override;

        void delete_command_buffers(std::vector<VkCommandBuffer>* buffers) override;

        VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, const std::vector<const vulkan::Shader*>& shaders,
                                   const vulkan::PipelineLayout& pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override;

public:
        Impl(vulkan::DepthBufferCount buffer_count, const vulkan::Swapchain& swapchain,
             const std::unordered_set<uint32_t>& attachment_family_indices, VkCommandPool command_pool,
             const vulkan::Device& device, double zoom);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(vulkan::DepthBufferCount buffer_count, const vulkan::Swapchain& swapchain,
           const std::unordered_set<uint32_t>& attachment_family_indices, VkCommandPool command_pool,
           const vulkan::Device& device, double zoom)
        : m_device(device), m_command_pool(command_pool)
{
        ASSERT(attachment_family_indices.size() > 0);

        zoom = std::max(zoom, 1.0);

        unsigned width = std::lround(swapchain.width() * zoom);
        unsigned height = std::lround(swapchain.height() * zoom);

        unsigned count = compute_buffer_count(buffer_count, swapchain);

        for (unsigned i = 0; i < count; ++i)
        {
                std::vector<VkFormat> depth_formats;
                if (!m_depth_attachments.empty())
                {
                        depth_formats = {m_depth_attachments[0].format()};
                }
                else
                {
                        depth_formats = DEPTH_IMAGE_FORMATS;
                }
                m_depth_attachments.emplace_back(m_device, attachment_family_indices, depth_formats, width, height);
        }

        VkFormat depth_format = m_depth_attachments[0].format();
        unsigned depth_width = m_depth_attachments[0].width();
        unsigned depth_height = m_depth_attachments[0].height();

        m_render_pass = vulkan_render_implementation::render_pass_depth(m_device, depth_format);

        std::vector<VkImageView> attachments(1);
        for (const vulkan::DepthAttachmentTexture& depth_attachment : m_depth_attachments)
        {
                attachments[0] = depth_attachment.image_view();

                m_framebuffers.push_back(create_framebuffer(m_device, m_render_pass, depth_width, depth_height, attachments));
        }

        check_buffers(m_depth_attachments);

        LOG(buffer_info(m_depth_attachments, zoom, width, height));
}

std::vector<VkCommandBuffer> Impl::create_command_buffers(const std::function<void(VkCommandBuffer buffer)>& commands)
{
        ASSERT(m_depth_attachments.size() > 0 && m_depth_attachments.size() == m_framebuffers.size());

        unsigned width = m_depth_attachments[0].width();
        unsigned height = m_depth_attachments[0].height();

        std::array<VkClearValue, 1> clear_values;
        clear_values[0] = vulkan::depth_stencil_clear_value();

        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.width = width;
        info.height = height;
        info.render_pass = m_render_pass;
        info.framebuffers.emplace(m_framebuffers);
        info.command_pool = m_command_pool;
        info.clear_values.emplace(clear_values);
        info.before_render_pass_commands = std::nullopt;
        info.render_pass_commands = commands;

        m_command_buffers.push_back(vulkan::create_command_buffers(info));

        return m_command_buffers.back().buffers();
}

void Impl::delete_command_buffers(std::vector<VkCommandBuffer>* buffers)
{
        delete_buffers(&m_command_buffers, buffers);
}

const vulkan::DepthAttachmentTexture* Impl::texture(unsigned index) const
{
        ASSERT(index < m_depth_attachments.size());

        // Указатель можно возвращать, так как m_depth_attachments
        // не меняется, а при работе Impl(Impl&&) сохранятся указатели,
        // так как std::vector(std::vector&&) сохраняет указатели.
        return &m_depth_attachments[index];
}

VkPipeline Impl::create_pipeline(VkPrimitiveTopology primitive_topology, const std::vector<const vulkan::Shader*>& shaders,
                                 const vulkan::PipelineLayout& pipeline_layout,
                                 const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                 const std::vector<VkVertexInputAttributeDescription>& vertex_attribute)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);
        ASSERT(m_depth_attachments.size() > 0 && m_depth_attachments.size() == m_framebuffers.size());

        unsigned width = m_depth_attachments[0].width();
        unsigned height = m_depth_attachments[0].height();

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = m_render_pass;
        info.sub_pass = 0;
        info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        info.sample_shading = false;
        info.pipeline_layout = pipeline_layout;
        info.width = width;
        info.height = height;
        info.primitive_topology = primitive_topology;
        info.shaders = &shaders;
        info.binding_descriptions = &vertex_binding;
        info.attribute_descriptions = &vertex_attribute;
        info.depth_bias = true;
        info.color_blend = false;

        m_pipelines.push_back(vulkan::create_graphics_pipeline(info));

        return m_pipelines.back();
}
}

namespace vulkan
{
std::unique_ptr<DepthBuffers> create_depth_buffers(DepthBufferCount buffer_count, const vulkan::Swapchain& swapchain,
                                                   const std::unordered_set<uint32_t>& attachment_family_indices,
                                                   VkCommandPool command_pool, const vulkan::Device& device, double zoom)
{
        return std::make_unique<Impl>(buffer_count, swapchain, attachment_family_indices, command_pool, device, zoom);
}
}
