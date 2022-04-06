/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "render_buffers.h"

#include <src/vulkan/create.h>

#include <array>

namespace ns::gpu::renderer
{
namespace
{
vulkan::handle::RenderPass create_render_pass_depth(
        const VkDevice device,
        const VkFormat depth_format,
        const VkSampleCountFlagBits sample_count)
{
        std::array<VkAttachmentDescription, 1> attachments = {};

        // Depth
        attachments[0].format = depth_format;
        attachments[0].samples = sample_count;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 0;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 0;
        subpass_description.pDepthStencilAttachment = &depth_reference;

        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_description;

        return vulkan::handle::RenderPass(device, create_info);
}

class Impl final : public RenderBuffers
{
        vulkan::handle::RenderPass render_pass_;
        std::vector<vulkan::handle::Framebuffer> framebuffers_;
        std::vector<VkFramebuffer> framebuffers_handles_;

        VkRenderPass render_pass() const override;
        const std::vector<VkFramebuffer>& framebuffers() const override;

public:
        Impl(RenderBuffers3D* render_buffers, const vulkan::Device& device);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(RenderBuffers3D* const render_buffers, const vulkan::Device& device)
{
        render_pass_ = create_render_pass_depth(device, render_buffers->depth_format(), render_buffers->sample_count());
        const std::size_t buffer_count = render_buffers->framebuffers().size();

        std::vector<VkImageView> attachments(1);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
                attachments[0] = render_buffers->depth_image_view(i);

                framebuffers_.push_back(create_framebuffer(
                        device, render_pass_, render_buffers->width(), render_buffers->height(), attachments));
                framebuffers_handles_.push_back(framebuffers_.back());
        }
}

VkRenderPass Impl::render_pass() const
{
        return render_pass_;
}

const std::vector<VkFramebuffer>& Impl::framebuffers() const
{
        ASSERT(framebuffers_.size() == framebuffers_handles_.size());
        return framebuffers_handles_;
}
}

std::unique_ptr<RenderBuffers> create_render_buffers(
        RenderBuffers3D* const render_buffers,
        const vulkan::Device& device)
{
        return std::make_unique<Impl>(render_buffers, device);
}
}
