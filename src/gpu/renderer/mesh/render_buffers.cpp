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

#include <src/vulkan/buffers.h>
#include <src/vulkan/create.h>

#include <array>

namespace ns::gpu::renderer
{
namespace
{
constexpr VkFormat COLOR_FORMAT_0 = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr VkFormat COLOR_FORMAT_1 = VK_FORMAT_R32G32B32A32_SFLOAT;

vulkan::handle::RenderPass create_render_pass(
        const VkDevice device,
        const VkFormat depth_format,
        const VkSampleCountFlagBits sample_count)
{
        std::array<VkAttachmentDescription, 3> attachments = {};

        // Color

        attachments[0].format = COLOR_FORMAT_0;
        attachments[0].samples = sample_count;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

        attachments[1].format = COLOR_FORMAT_1;
        attachments[1].samples = sample_count;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

        // Depth

        attachments[2].format = depth_format;
        attachments[2].samples = sample_count;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        //

        std::array<VkAttachmentReference, 2> color_references;
        color_references[0] = {};
        color_references[0].attachment = 0;
        color_references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_references[1] = {};
        color_references[1].attachment = 1;
        color_references[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 0;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = color_references.size();
        subpass_description.pColorAttachments = color_references.data();
        subpass_description.pDepthStencilAttachment = &depth_reference;

        std::array<VkSubpassDependency, 1> subpass_dependencies = {};
        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0;
        subpass_dependencies[0].dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_description;
        create_info.dependencyCount = subpass_dependencies.size();
        create_info.pDependencies = subpass_dependencies.data();

        return vulkan::handle::RenderPass(device, create_info);
}

VkClearValue create_color_clear_value()
{
        static_assert(COLOR_FORMAT_0 == VK_FORMAT_R32G32B32A32_SFLOAT);
        static_assert(COLOR_FORMAT_1 == VK_FORMAT_R32G32B32A32_SFLOAT);

        VkClearValue res;
        res.color.float32[0] = 0;
        res.color.float32[1] = 0;
        res.color.float32[2] = 0;
        res.color.float32[3] = 0;
        return res;
}

struct ColorImages final
{
        vulkan::ImageWithMemory image_0;
        vulkan::ImageWithMemory image_1;

        ColorImages(
                RenderBuffers3D* const render_buffers,
                const vulkan::Device& device,
                const std::vector<std::uint32_t>& family_indices)
                : image_0(
                        device,
                        family_indices,
                        {COLOR_FORMAT_0},
                        render_buffers->sample_count(),
                        VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(render_buffers->width(), render_buffers->height()),
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
                  image_1(device,
                          family_indices,
                          {COLOR_FORMAT_1},
                          render_buffers->sample_count(),
                          VK_IMAGE_TYPE_2D,
                          vulkan::make_extent(render_buffers->width(), render_buffers->height()),
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT)
        {
        }
};

class Impl final : public RenderBuffers
{
        std::vector<ColorImages> color_attachments_;
        std::vector<ImageViews> image_views_;

        vulkan::handle::RenderPass render_pass_;
        std::vector<vulkan::handle::Framebuffer> framebuffers_;
        std::vector<VkFramebuffer> framebuffers_handles_;

        VkRenderPass render_pass() const override;
        const std::vector<VkFramebuffer>& framebuffers() const override;
        std::vector<VkClearValue> clear_values() const override;
        const std::vector<RenderBuffers::ImageViews>& image_views() const override;

public:
        Impl(RenderBuffers3D* render_buffers,
             const vulkan::Device& device,
             const std::vector<std::uint32_t>& family_indices);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(
        RenderBuffers3D* const render_buffers,
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices)
{
        const std::size_t buffer_count = render_buffers->framebuffers().size();

        color_attachments_.reserve(buffer_count);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
                color_attachments_.emplace_back(render_buffers, device, family_indices);
        }

        image_views_.reserve(buffer_count);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
                image_views_.push_back(
                        {.image_0 = &color_attachments_[i].image_0.image_view(),
                         .image_1 = &color_attachments_[i].image_1.image_view()});
        }

        render_pass_ = create_render_pass(device, render_buffers->depth_format(), render_buffers->sample_count());

        std::vector<VkImageView> attachments(3);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
                attachments[0] = color_attachments_[i].image_0.image_view();
                attachments[1] = color_attachments_[i].image_1.image_view();
                attachments[2] = render_buffers->depth_image_view(i);

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

std::vector<VkClearValue> Impl::clear_values() const
{
        ASSERT(color_attachments_.size() == 2);
        std::vector<VkClearValue> clear_values(3);
        clear_values[0] = create_color_clear_value();
        clear_values[1] = create_color_clear_value();
        return clear_values;
}

const std::vector<RenderBuffers::ImageViews>& Impl::image_views() const
{
        return image_views_;
}
}

std::unique_ptr<RenderBuffers> create_render_buffers(
        RenderBuffers3D* const render_buffers,
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices)
{
        return std::make_unique<Impl>(render_buffers, device, family_indices);
}
}
