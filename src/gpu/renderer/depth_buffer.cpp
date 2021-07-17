/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/vulkan/create.h>
#include <src/vulkan/print.h>

#include <algorithm>
#include <sstream>

namespace ns::gpu::renderer
{
namespace
{
// clang-format off
constexpr std::initializer_list<VkFormat> DEPTH_IMAGE_FORMATS =
{
        VK_FORMAT_D32_SFLOAT
};
// clang-format on

constexpr VkSampleCountFlagBits SAMPLE_COUNT = VK_SAMPLE_COUNT_1_BIT;
constexpr VkImageLayout IMAGE_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

vulkan::RenderPass create_render_pass_depth(VkDevice device, VkFormat depth_format)
{
        std::array<VkAttachmentDescription, 1> attachments = {};

        // Depth
        attachments[0].format = depth_format;
        attachments[0].samples = SAMPLE_COUNT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = IMAGE_LAYOUT;

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

void check_buffers(const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        ASSERT(std::all_of(
                depth.cbegin(), depth.cend(),
                [](const vulkan::DepthImageWithMemory& d)
                {
                        return d.usage() & VK_IMAGE_USAGE_SAMPLED_BIT;
                }));
        ASSERT(std::all_of(
                depth.cbegin(), depth.cend(),
                [](const vulkan::DepthImageWithMemory& d)
                {
                        return d.sample_count() == SAMPLE_COUNT;
                }));

        if (depth.empty())
        {
                error("No depth attachment");
        }

        if (!std::all_of(
                    depth.cbegin(), depth.cend(),
                    [&](const vulkan::DepthImageWithMemory& d)
                    {
                            return d.format() == depth[0].format();
                    }))
        {
                error("Depth attachments must have the same format");
        }

        if (!std::all_of(
                    depth.cbegin(), depth.cend(),
                    [&](const vulkan::DepthImageWithMemory& d)
                    {
                            return d.width() == depth[0].width() && d.height() == depth[0].height();
                    }))
        {
                error("Depth attachments must have the same size");
        }
}

std::string buffer_info(
        const std::vector<vulkan::DepthImageWithMemory>& depth,
        double zoom,
        unsigned width,
        unsigned height)
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

class Impl final : public DepthBuffers
{
        const vulkan::Device& m_device;

        //

        std::vector<vulkan::DepthImageWithMemory> m_depth_attachments;
        vulkan::RenderPass m_render_pass;
        std::vector<vulkan::Framebuffer> m_framebuffers;
        std::vector<VkFramebuffer> m_framebuffers_handles;
        std::vector<VkClearValue> m_clear_values;

        //

        const vulkan::DepthImageWithMemory* texture(unsigned index) const override;
        unsigned width() const override;
        unsigned height() const override;
        VkRenderPass render_pass() const override;
        VkSampleCountFlagBits sample_count() const override;
        const std::vector<VkFramebuffer>& framebuffers() const override;
        const std::vector<VkClearValue>& clear_values() const override;

public:
        Impl(unsigned buffer_count,
             const std::vector<uint32_t>& attachment_family_indices,
             VkCommandPool graphics_command_pool,
             VkQueue graphics_queue,
             const vulkan::Device& device,
             unsigned width,
             unsigned height,
             double zoom);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(
        unsigned buffer_count,
        const std::vector<uint32_t>& attachment_family_indices,
        VkCommandPool graphics_command_pool,
        VkQueue graphics_queue,
        const vulkan::Device& device,
        unsigned width,
        unsigned height,
        double zoom)
        : m_device(device)
{
        ASSERT(!attachment_family_indices.empty());

        zoom = std::max(zoom, 1.0);
        width = std::lround(width * zoom);
        height = std::lround(height * zoom);

        for (unsigned i = 0; i < buffer_count; ++i)
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
                m_depth_attachments.emplace_back(
                        m_device, attachment_family_indices, depth_formats, SAMPLE_COUNT, width, height,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, IMAGE_LAYOUT,
                        graphics_command_pool, graphics_queue);
        }

        VkFormat depth_format = m_depth_attachments[0].format();
        unsigned depth_width = m_depth_attachments[0].width();
        unsigned depth_height = m_depth_attachments[0].height();

        m_render_pass = create_render_pass_depth(m_device, depth_format);

        std::vector<VkImageView> attachments(1);
        for (const vulkan::DepthImageWithMemory& depth_attachment : m_depth_attachments)
        {
                attachments[0] = depth_attachment.image_view();

                m_framebuffers.push_back(
                        create_framebuffer(m_device, m_render_pass, depth_width, depth_height, attachments));
                m_framebuffers_handles.push_back(m_framebuffers.back());
        }

        m_clear_values.push_back(vulkan::depth_stencil_clear_value());

        check_buffers(m_depth_attachments);

        LOG(buffer_info(m_depth_attachments, zoom, width, height));
}

const vulkan::DepthImageWithMemory* Impl::texture(unsigned index) const
{
        ASSERT(index < m_depth_attachments.size());

        // Указатель можно возвращать, так как m_depth_attachments
        // не меняется, а при работе Impl(Impl&&) сохранятся указатели,
        // так как std::vector(std::vector&&) сохраняет указатели.
        return &m_depth_attachments[index];
}

unsigned Impl::width() const
{
        ASSERT(!m_depth_attachments.empty() && m_depth_attachments.size() == m_framebuffers.size());
        return m_depth_attachments[0].width();
}

unsigned Impl::height() const
{
        ASSERT(!m_depth_attachments.empty() && m_depth_attachments.size() == m_framebuffers.size());
        return m_depth_attachments[0].height();
}

VkRenderPass Impl::render_pass() const
{
        return m_render_pass;
}

VkSampleCountFlagBits Impl::sample_count() const
{
        return SAMPLE_COUNT;
}

const std::vector<VkFramebuffer>& Impl::framebuffers() const
{
        ASSERT(!m_depth_attachments.empty() && m_depth_attachments.size() == m_framebuffers.size());
        ASSERT(m_framebuffers.size() == m_framebuffers_handles.size());
        return m_framebuffers_handles;
}

const std::vector<VkClearValue>& Impl::clear_values() const
{
        ASSERT(m_clear_values.size() == 1);
        return m_clear_values;
}
}

std::unique_ptr<DepthBuffers> create_depth_buffers(
        unsigned buffer_count,
        const std::vector<uint32_t>& attachment_family_indices,
        VkCommandPool graphics_command_pool,
        VkQueue graphics_queue,
        const vulkan::Device& device,
        unsigned width,
        unsigned height,
        double zoom)
{
        return std::make_unique<Impl>(
                buffer_count, attachment_family_indices, graphics_command_pool, graphics_queue, device, width, height,
                zoom);
}
}
