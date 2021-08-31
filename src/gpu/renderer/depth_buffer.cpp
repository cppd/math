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
#include <array>
#include <sstream>

namespace ns::gpu::renderer
{
namespace
{
// clang-format off
constexpr std::array DEPTH_IMAGE_FORMATS = std::to_array<VkFormat>
({
        VK_FORMAT_D32_SFLOAT
});
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
        const vulkan::Device& device_;

        //

        std::vector<vulkan::DepthImageWithMemory> depth_attachments_;
        vulkan::RenderPass render_pass_;
        std::vector<vulkan::Framebuffer> framebuffers_;
        std::vector<VkFramebuffer> framebuffers_handles_;
        std::vector<VkClearValue> clear_values_;

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
        Impl(Impl&&) = delete;
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
        : device_(device)
{
        ASSERT(!attachment_family_indices.empty());

        zoom = std::max(zoom, 1.0);
        width = std::lround(width * zoom);
        height = std::lround(height * zoom);

        std::vector<VkFormat> depth_formats(std::cbegin(DEPTH_IMAGE_FORMATS), std::cend(DEPTH_IMAGE_FORMATS));
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                if (i == 1)
                {
                        depth_formats = {depth_attachments_[0].format()};
                }
                depth_attachments_.emplace_back(
                        device_, attachment_family_indices, depth_formats, SAMPLE_COUNT, width, height,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, IMAGE_LAYOUT,
                        graphics_command_pool, graphics_queue);
        }

        VkFormat depth_format = depth_attachments_[0].format();
        unsigned depth_width = depth_attachments_[0].width();
        unsigned depth_height = depth_attachments_[0].height();

        render_pass_ = create_render_pass_depth(device_, depth_format);

        std::vector<VkImageView> attachments(1);
        for (const vulkan::DepthImageWithMemory& depth_attachment : depth_attachments_)
        {
                attachments[0] = depth_attachment.image_view();

                framebuffers_.push_back(
                        create_framebuffer(device_, render_pass_, depth_width, depth_height, attachments));
                framebuffers_handles_.push_back(framebuffers_.back());
        }

        clear_values_.push_back(vulkan::create_depth_stencil_clear_value());

        check_buffers(depth_attachments_);

        LOG(buffer_info(depth_attachments_, zoom, width, height));
}

const vulkan::DepthImageWithMemory* Impl::texture(unsigned index) const
{
        ASSERT(index < depth_attachments_.size());

        return &depth_attachments_[index];
}

unsigned Impl::width() const
{
        ASSERT(!depth_attachments_.empty() && depth_attachments_.size() == framebuffers_.size());
        return depth_attachments_[0].width();
}

unsigned Impl::height() const
{
        ASSERT(!depth_attachments_.empty() && depth_attachments_.size() == framebuffers_.size());
        return depth_attachments_[0].height();
}

VkRenderPass Impl::render_pass() const
{
        return render_pass_;
}

VkSampleCountFlagBits Impl::sample_count() const
{
        return SAMPLE_COUNT;
}

const std::vector<VkFramebuffer>& Impl::framebuffers() const
{
        ASSERT(!depth_attachments_.empty() && depth_attachments_.size() == framebuffers_.size());
        ASSERT(framebuffers_.size() == framebuffers_handles_.size());
        return framebuffers_handles_;
}

const std::vector<VkClearValue>& Impl::clear_values() const
{
        ASSERT(clear_values_.size() == 1);
        return clear_values_;
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
