/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "buffer_info.h"
#include "image_commands.h"
#include "render_pass.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/create.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace ns::view::view
{
namespace
{
constexpr VkImageLayout COLOR_ATTACHMENT_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
constexpr VkImageLayout DEPTH_ATTACHMENT_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

class Impl3D : public gpu::RenderBuffers3D
{
        [[nodiscard]] virtual const vulkan::RenderPass& render_pass_3d() const = 0;
        [[nodiscard]] virtual const std::vector<VkFramebuffer>& framebuffers_3d() const = 0;

        //

        [[nodiscard]] const vulkan::RenderPass& render_pass() const final
        {
                return render_pass_3d();
        }

        [[nodiscard]] const std::vector<VkFramebuffer>& framebuffers() const final
        {
                return framebuffers_3d();
        }

protected:
        ~Impl3D()
        {
        }
};

class Impl2D : public gpu::RenderBuffers2D
{
        [[nodiscard]] virtual const vulkan::RenderPass& render_pass_2d() const = 0;
        [[nodiscard]] virtual const std::vector<VkFramebuffer>& framebuffers_2d() const = 0;

        //

        [[nodiscard]] const vulkan::RenderPass& render_pass() const final
        {
                return render_pass_2d();
        }

        [[nodiscard]] const std::vector<VkFramebuffer>& framebuffers() const final
        {
                return framebuffers_2d();
        }

protected:
        ~Impl2D()
        {
        }
};

class Impl final : public RenderBuffers, public Impl3D, public Impl2D
{
        const VkFormat format_;
        const unsigned width_;
        const unsigned height_;

        std::vector<vulkan::ImageWithMemory> color_attachments_;
        std::vector<vulkan::DepthImageWithMemory> depth_attachments_;

        std::vector<VkImageView> color_attachment_image_views_;

        vulkan::RenderPass render_pass_3d_;
        std::vector<vulkan::handle::Framebuffer> framebuffers_3d_;
        std::vector<VkFramebuffer> framebuffers_handles_3d_;

        vulkan::RenderPass render_pass_3d_clear_;
        std::vector<vulkan::handle::Framebuffer> framebuffers_3d_clear_;
        std::vector<VkFramebuffer> framebuffers_handles_3d_clear_;

        vulkan::RenderPass render_pass_2d_;
        std::vector<vulkan::handle::Framebuffer> framebuffers_2d_;
        std::vector<VkFramebuffer> framebuffers_handles_2d_;

        void create_buffers(
                const vulkan::Device& device,
                std::span<const VkFormat> depth_formats,
                unsigned buffer_count,
                VkSampleCountFlagBits sample_count,
                const std::vector<std::uint32_t>& attachment_family_indices);

        [[nodiscard]] RenderBuffers3D& buffers_3d() override;
        [[nodiscard]] RenderBuffers2D& buffers_2d() override;

        [[nodiscard]] unsigned width() const override;
        [[nodiscard]] unsigned height() const override;
        [[nodiscard]] VkFormat color_format() const override;
        [[nodiscard]] VkFormat depth_format() const override;
        [[nodiscard]] VkImageView depth_image_view(unsigned index) const override;
        [[nodiscard]] VkSampleCountFlagBits sample_count() const override;
        [[nodiscard]] const std::vector<VkImageView>& image_views() const override;

        [[nodiscard]] VkRenderPass render_pass_clear() const override;
        [[nodiscard]] const std::vector<VkFramebuffer>& framebuffers_clear() const override;
        [[nodiscard]] std::vector<VkClearValue> clear_values(const numerical::Vector<3, float>& rgb) const override;

        void commands_color_resolve(
                VkCommandBuffer command_buffer,
                VkImage image,
                VkImageLayout image_layout,
                const numerical::Region<2, int>& rectangle,
                unsigned index) const override;

        void commands_depth_copy(
                VkCommandBuffer command_buffer,
                VkImage image,
                VkImageLayout image_layout,
                const numerical::Region<2, int>& rectangle,
                unsigned index) const override;

        //

        [[nodiscard]] const vulkan::RenderPass& render_pass_3d() const override;
        [[nodiscard]] const std::vector<VkFramebuffer>& framebuffers_3d() const override;

        [[nodiscard]] const vulkan::RenderPass& render_pass_2d() const override;
        [[nodiscard]] const std::vector<VkFramebuffer>& framebuffers_2d() const override;

public:
        Impl(unsigned buffer_count,
             VkFormat color_format,
             const std::span<const VkFormat>& depth_formats,
             unsigned width,
             unsigned height,
             const std::vector<std::uint32_t>& family_indices,
             const vulkan::Device& device,
             VkSampleCountFlagBits sample_count);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(
        const unsigned buffer_count,
        const VkFormat color_format,
        const std::span<const VkFormat>& depth_formats,
        const unsigned width,
        const unsigned height,
        const std::vector<std::uint32_t>& family_indices,
        const vulkan::Device& device,
        const VkSampleCountFlagBits sample_count)
        : format_(color_format),
          width_(width),
          height_(height)
{
        if (buffer_count < 1)
        {
                error("Buffer count " + std::to_string(buffer_count) + " must be positive");
        }

        if (width < 1 || height < 1)
        {
                error("Width " + std::to_string(width) + " and height " + std::to_string(height) + " must be positive");
        }

        create_buffers(device, depth_formats, buffer_count, sample_count, family_indices);

        render_buffer_check(color_attachments_, depth_attachments_);

        LOG(render_buffer_info(color_attachments_, depth_attachments_));
}

void Impl::create_buffers(
        const vulkan::Device& device,
        const std::span<const VkFormat> depth_formats,
        const unsigned buffer_count,
        const VkSampleCountFlagBits sample_count,
        const std::vector<std::uint32_t>& family_indices)
{
        const std::vector<VkFormat> color_format = {format_};

        for (unsigned i = 0; i < buffer_count; ++i)
        {
                color_attachments_.emplace_back(
                        device, family_indices, color_format, sample_count, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(width_, height_),
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
                color_attachment_image_views_.push_back(color_attachments_.back().image_view().handle());

                std::vector<VkFormat> formats;
                if (!depth_attachments_.empty())
                {
                        formats = {depth_attachments_[0].image().format()};
                }
                else
                {
                        formats = {depth_formats.begin(), depth_formats.end()};
                }
                depth_attachments_.emplace_back(
                        device, family_indices, formats, sample_count, width_, height_,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        }

        const VkFormat depth_format = depth_attachments_[0].image().format();

        std::vector<VkImageView> attachments;

        render_pass_3d_ = render_pass_color_depth(device.handle(), format_, depth_format, sample_count, false);
        render_pass_3d_clear_ = render_pass_color_depth(device.handle(), format_, depth_format, sample_count, true);

        attachments.resize(2);
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                attachments[0] = color_attachments_[i].image_view().handle();
                attachments[1] = depth_attachments_[i].image_view().handle();

                framebuffers_3d_.push_back(
                        vulkan::create_framebuffer(
                                device.handle(), render_pass_3d_.handle(), width_, height_, attachments));
                framebuffers_handles_3d_.push_back(framebuffers_3d_.back());

                framebuffers_3d_clear_.push_back(
                        vulkan::create_framebuffer(
                                device.handle(), render_pass_3d_clear_.handle(), width_, height_, attachments));
                framebuffers_handles_3d_clear_.push_back(framebuffers_3d_clear_.back());
        }

        render_pass_2d_ = render_pass_color(device.handle(), format_, sample_count);

        attachments.resize(1);
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                attachments[0] = color_attachments_[i].image_view().handle();

                framebuffers_2d_.push_back(
                        vulkan::create_framebuffer(
                                device.handle(), render_pass_2d_.handle(), width_, height_, attachments));
                framebuffers_handles_2d_.push_back(framebuffers_2d_.back());
        }
}

gpu::RenderBuffers3D& Impl::buffers_3d()
{
        return *this;
}

gpu::RenderBuffers2D& Impl::buffers_2d()
{
        return *this;
}

const vulkan::RenderPass& Impl::render_pass_3d() const
{
        return render_pass_3d_;
}

const std::vector<VkFramebuffer>& Impl::framebuffers_3d() const
{
        ASSERT(!framebuffers_3d_.empty() && framebuffers_3d_.size() == framebuffers_handles_3d_.size());
        return framebuffers_handles_3d_;
}

const vulkan::RenderPass& Impl::render_pass_2d() const
{
        return render_pass_2d_;
}

const std::vector<VkFramebuffer>& Impl::framebuffers_2d() const
{
        ASSERT(!framebuffers_2d_.empty() && framebuffers_2d_.size() == framebuffers_handles_2d_.size());
        return framebuffers_handles_2d_;
}

VkRenderPass Impl::render_pass_clear() const
{
        return render_pass_3d_clear_.handle();
}

const std::vector<VkFramebuffer>& Impl::framebuffers_clear() const
{
        ASSERT(!framebuffers_3d_clear_.empty()
               && framebuffers_3d_clear_.size() == framebuffers_handles_3d_clear_.size());
        return framebuffers_handles_3d_clear_;
}

std::vector<VkClearValue> Impl::clear_values(const numerical::Vector<3, float>& rgb) const
{
        std::vector<VkClearValue> res(2);
        res[0] = vulkan::create_color_clear_value(format_, rgb);
        res[1] = vulkan::create_depth_stencil_clear_value();
        return res;
}

unsigned Impl::width() const
{
        return width_;
}

unsigned Impl::height() const
{
        return height_;
}

VkFormat Impl::color_format() const
{
        return format_;
}

VkFormat Impl::depth_format() const
{
        ASSERT(!depth_attachments_.empty());
        return depth_attachments_[0].image().format();
}

VkImageView Impl::depth_image_view(const unsigned index) const
{
        ASSERT(index < depth_attachments_.size());
        return depth_attachments_[index].image_view().handle();
}

VkSampleCountFlagBits Impl::sample_count() const
{
        return color_attachments_[0].image().sample_count();
}

const std::vector<VkImageView>& Impl::image_views() const
{
        ASSERT(color_attachments_.size() == color_attachment_image_views_.size());
        return color_attachment_image_views_;
}

void Impl::commands_color_resolve(
        const VkCommandBuffer command_buffer,
        const VkImage image,
        const VkImageLayout image_layout,
        const numerical::Region<2, int>& rectangle,
        const unsigned index) const
{
        ASSERT(index < color_attachments_.size());
        ASSERT(color_attachments_[index].image().sample_count() != VK_SAMPLE_COUNT_1_BIT);

        commands_image_resolve(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0,
                color_attachments_[index].image().handle(), COLOR_ATTACHMENT_IMAGE_LAYOUT, image, image_layout,
                rectangle);
}

void Impl::commands_depth_copy(
        const VkCommandBuffer command_buffer,
        const VkImage image,
        const VkImageLayout image_layout,
        const numerical::Region<2, int>& rectangle,
        const unsigned index) const
{
        ASSERT(image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ASSERT(index < depth_attachments_.size());

        commands_image_copy(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 0,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, depth_attachments_[index].image().handle(),
                DEPTH_ATTACHMENT_IMAGE_LAYOUT, image, image_layout, rectangle);
}
}

std::unique_ptr<RenderBuffers> create_render_buffers(
        const unsigned buffer_count,
        const VkFormat color_format,
        const std::span<const VkFormat>& depth_formats,
        const unsigned width,
        const unsigned height,
        const std::vector<std::uint32_t>& family_indices,
        const vulkan::Device& device,
        const VkSampleCountFlagBits sample_count)
{
        return std::make_unique<Impl>(
                buffer_count, color_format, depth_formats, width, height, family_indices, device, sample_count);
}
}
