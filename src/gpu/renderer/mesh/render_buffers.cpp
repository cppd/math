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
vulkan::RenderPass create_render_pass(
        const VkDevice device,
        const VkFormat depth_format,
        const VkSampleCountFlagBits sample_count,
        const std::vector<vulkan::ImageWithMemory>& images)
{
        std::vector<VkAttachmentDescription> attachments(images.size() + 1);

        // Color

        for (std::size_t i = 0; i < images.size(); ++i)
        {
                ASSERT(images[i].image_view().sample_count() == sample_count);
                attachments[i] = {};
                attachments[i].format = images[i].image_view().format();
                attachments[i].samples = sample_count;
                attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[i].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
        }

        // Depth

        attachments.back() = {};
        attachments.back().format = depth_format;
        attachments.back().samples = sample_count;
        attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments.back().initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.back().finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        //

        std::vector<VkAttachmentReference> color_references(images.size());

        for (std::size_t i = 0; i < images.size(); ++i)
        {
                color_references[i] = {};
                color_references[i].attachment = i;
                color_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = images.size();
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

        return vulkan::RenderPass(device, create_info);
}

class Impl final : public RenderBuffers
{
        unsigned width_;
        unsigned height_;
        VkSampleCountFlagBits sample_count_;
        vulkan::RenderPass render_pass_;
        std::vector<vulkan::handle::Framebuffer> framebuffers_;
        std::vector<VkFramebuffer> framebuffers_handles_;
        std::vector<VkClearValue> clear_values_;

        unsigned width() const override;
        unsigned height() const override;
        VkSampleCountFlagBits sample_count() const override;
        const vulkan::RenderPass& render_pass() const override;
        const std::vector<VkFramebuffer>& framebuffers() const override;
        const std::vector<VkClearValue>& clear_values() const override;

public:
        Impl(const RenderBuffers3D* render_buffers, const Opacity& opacity, VkDevice device);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(const RenderBuffers3D* const render_buffers, const Opacity& opacity, const VkDevice device)
        : width_(render_buffers->width()),
          height_(render_buffers->height()),
          sample_count_(render_buffers->sample_count())
{
        ASSERT(render_buffers->framebuffers().size() == 1);

        const std::vector<vulkan::ImageWithMemory>& images = opacity.images();

        ASSERT(images.size() == 2);
        ASSERT(images[0].image_view().format() == VK_FORMAT_R32G32B32A32_SFLOAT);
        ASSERT(images[1].image_view().format() == VK_FORMAT_R32G32B32A32_SFLOAT);

        ASSERT(std::all_of(
                images.cbegin(), images.cend(),
                [&](const vulkan::ImageWithMemory& image)
                {
                        return render_buffers->sample_count() == image.image_view().sample_count();
                }));

        const std::size_t buffer_count = render_buffers->framebuffers().size();

        render_pass_ =
                create_render_pass(device, render_buffers->depth_format(), render_buffers->sample_count(), images);

        std::vector<VkImageView> attachments(images.size() + 1);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
                for (std::size_t image_index = 0; image_index < images.size(); ++image_index)
                {
                        attachments[image_index] = images[image_index].image_view();
                }
                attachments.back() = render_buffers->depth_image_view(i);

                framebuffers_.push_back(vulkan::create_framebuffer(
                        device, render_pass_, render_buffers->width(), render_buffers->height(), attachments));
                framebuffers_handles_.push_back(framebuffers_.back());
        }

        clear_values_ = opacity.clear_values();
        clear_values_.emplace_back(); // depth
}

unsigned Impl::width() const
{
        return width_;
}

unsigned Impl::height() const
{
        return height_;
}

VkSampleCountFlagBits Impl::sample_count() const
{
        return sample_count_;
}

const vulkan::RenderPass& Impl::render_pass() const
{
        return render_pass_;
}

const std::vector<VkFramebuffer>& Impl::framebuffers() const
{
        ASSERT(framebuffers_.size() == framebuffers_handles_.size());
        return framebuffers_handles_;
}

const std::vector<VkClearValue>& Impl::clear_values() const
{
        return clear_values_;
}
}

std::unique_ptr<RenderBuffers> create_render_buffers(
        const RenderBuffers3D* const render_buffers,
        const Opacity& opacity,
        const VkDevice device)
{
        return std::make_unique<Impl>(render_buffers, opacity, device);
}
}
