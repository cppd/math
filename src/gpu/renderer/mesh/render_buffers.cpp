/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/error.h>
#include <src/gpu/render_buffers.h>
#include <src/gpu/renderer/buffers/opacity.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/create.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
void check_opacity_images([[maybe_unused]] const RenderBuffers3D& render_buffers, const Opacity& opacity)
{
        const std::vector<vulkan::ImageWithMemory>& images = opacity.images();

        ASSERT(images.size() == 2 || images.size() == 4);

        ASSERT(images[0].image_view().format() == VK_FORMAT_R32G32_UINT);
        ASSERT(images[1].image_view().format() == VK_FORMAT_R32G32B32A32_SFLOAT);

        if (images.size() == 4)
        {
                ASSERT(images[2].image_view().format() == VK_FORMAT_R32G32B32A32_SFLOAT);
                ASSERT(images[3].image_view().format() == VK_FORMAT_R32G32_SFLOAT);
        }

        ASSERT(std::ranges::all_of(
                images,
                [&](const vulkan::ImageWithMemory& image)
                {
                        return render_buffers.sample_count() == image.image_view().sample_count();
                }));
}

vulkan::RenderPass create_render_pass(
        const VkDevice device,
        const VkFormat depth_format,
        const VkSampleCountFlagBits sample_count,
        const std::vector<vulkan::ImageWithMemory>& images)
{
        const std::vector<VkAttachmentDescription> attachments = [&]
        {
                std::vector<VkAttachmentDescription> res(images.size() + 1);

                // Color
                for (std::size_t i = 0; i < images.size(); ++i)
                {
                        ASSERT(images[i].image_view().sample_count() == sample_count);
                        res[i].format = images[i].image_view().format();
                        res[i].samples = sample_count;
                        res[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        res[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                        res[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                        res[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                        res[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        res[i].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
                }

                // Depth
                res.back().format = depth_format;
                res.back().samples = sample_count;
                res.back().loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                res.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                res.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                res.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                res.back().initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                res.back().finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                return res;
        }();

        const std::vector<VkAttachmentReference> color_references = [&]
        {
                std::vector<VkAttachmentReference> res(images.size());
                for (std::size_t i = 0; i < images.size(); ++i)
                {
                        res[i].attachment = i;
                        res[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                return res;
        }();

        const VkAttachmentReference depth_reference = [&]
        {
                VkAttachmentReference res = {};
                res.attachment = images.size();
                res.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                return res;
        }();

        const VkSubpassDescription subpass_description = [&]
        {
                VkSubpassDescription res = {};
                res.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                res.colorAttachmentCount = color_references.size();
                res.pColorAttachments = color_references.data();
                res.pDepthStencilAttachment = &depth_reference;
                return res;
        }();

        const std::array<VkSubpassDependency, 1> subpass_dependencies = [&]
        {
                std::array<VkSubpassDependency, 1> res = {};
                res[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                res[0].dstSubpass = 0;
                res[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].srcAccessMask = 0;
                res[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                return res;
        }();

        const VkRenderPassCreateInfo create_info = [&]
        {
                VkRenderPassCreateInfo res = {};
                res.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                res.attachmentCount = attachments.size();
                res.pAttachments = attachments.data();
                res.subpassCount = 1;
                res.pSubpasses = &subpass_description;
                res.dependencyCount = subpass_dependencies.size();
                res.pDependencies = subpass_dependencies.data();
                return res;
        }();

        return {device, create_info};
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

        [[nodiscard]] unsigned width() const override;
        [[nodiscard]] unsigned height() const override;
        [[nodiscard]] VkSampleCountFlagBits sample_count() const override;
        [[nodiscard]] const vulkan::RenderPass& render_pass() const override;
        [[nodiscard]] const std::vector<VkFramebuffer>& framebuffers() const override;
        [[nodiscard]] const std::vector<VkClearValue>& clear_values() const override;

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

        check_opacity_images(*render_buffers, opacity);

        const std::vector<vulkan::ImageWithMemory>& images = opacity.images();

        render_pass_ =
                create_render_pass(device, render_buffers->depth_format(), render_buffers->sample_count(), images);

        const std::size_t buffer_count = render_buffers->framebuffers().size();

        framebuffers_.reserve(buffer_count);
        framebuffers_handles_.reserve(buffer_count);
        std::vector<VkImageView> attachments(images.size() + 1);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
                for (std::size_t image_index = 0; image_index < images.size(); ++image_index)
                {
                        attachments[image_index] = images[image_index].image_view().handle();
                }
                attachments.back() = render_buffers->depth_image_view(i);

                framebuffers_.push_back(
                        vulkan::create_framebuffer(
                                device, render_pass_.handle(), render_buffers->width(), render_buffers->height(),
                                attachments));
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
