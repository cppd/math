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

#include "render_buffers.h"

#include "buffer_info.h"
#include "commands.h"
#include "render_pass.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/create.h>
#include <src/vulkan/query.h>

namespace ns::view
{
namespace
{
constexpr VkImageLayout COLOR_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
constexpr VkImageLayout DEPTH_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

class Impl3D : public gpu::RenderBuffers3D
{
        virtual VkRenderPass render_pass_3d() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers_3d() const = 0;
        virtual VkRenderPass render_pass_clear_3d() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers_clear_3d() const = 0;
        virtual std::vector<VkClearValue> clear_values_3d(const Vector<3, float>& rgb) const = 0;

        //

        VkRenderPass render_pass() const final
        {
                return render_pass_3d();
        }

        const std::vector<VkFramebuffer>& framebuffers() const final
        {
                return framebuffers_3d();
        }

        VkRenderPass render_pass_clear() const final
        {
                return render_pass_clear_3d();
        }

        const std::vector<VkFramebuffer>& framebuffers_clear() const final
        {
                return framebuffers_clear_3d();
        }

        std::vector<VkClearValue> clear_values(const Vector<3, float>& rgb) const final
        {
                return clear_values_3d(rgb);
        }

protected:
        ~Impl3D() override = default;
};

class Impl2D : public gpu::RenderBuffers2D
{
        virtual VkRenderPass render_pass_2d() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers_2d() const = 0;

        //

        VkRenderPass render_pass() const final
        {
                return render_pass_2d();
        }

        const std::vector<VkFramebuffer>& framebuffers() const final
        {
                return framebuffers_2d();
        }

protected:
        ~Impl2D() override = default;
};

class Impl final : public RenderBuffers, public Impl3D, public Impl2D
{
        const VkFormat m_format;
        const unsigned m_width;
        const unsigned m_height;

        std::vector<vulkan::ImageWithMemory> m_color_attachments;
        std::vector<vulkan::DepthImageWithMemory> m_depth_attachments;

        std::vector<VkImageView> m_color_attachment_image_views;

        vulkan::RenderPass m_3d_render_pass;
        vulkan::RenderPass m_3d_render_pass_clear;
        std::vector<vulkan::Framebuffer> m_3d_framebuffers;
        std::vector<vulkan::Framebuffer> m_3d_framebuffers_clear;
        std::vector<VkFramebuffer> m_3d_framebuffers_handles;
        std::vector<VkFramebuffer> m_3d_framebuffers_handles_clear;

        vulkan::RenderPass m_2d_render_pass;
        std::vector<vulkan::Framebuffer> m_2d_framebuffers;
        std::vector<VkFramebuffer> m_2d_framebuffers_handles;

        void create_buffers(
                const vulkan::Device& device,
                const std::span<const VkFormat>& depth_formats,
                unsigned buffer_count,
                VkSampleCountFlagBits sample_count,
                const std::vector<uint32_t>& attachment_family_indices);

        RenderBuffers3D& buffers_3d() override;
        RenderBuffers2D& buffers_2d() override;

        unsigned width() const override;
        unsigned height() const override;
        VkFormat color_format() const override;
        VkFormat depth_format() const override;
        VkSampleCountFlagBits sample_count() const override;
        const std::vector<VkImageView>& image_views() const override;

        void commands_color_resolve(
                VkCommandBuffer command_buffer,
                VkImage image,
                VkImageLayout layout,
                const Region<2, int>& rectangle,
                unsigned index) const override;

        void commands_depth_copy(
                VkCommandBuffer command_buffer,
                VkImage image,
                VkImageLayout layout,
                const Region<2, int>& rectangle,
                unsigned index) const override;

        //

        VkRenderPass render_pass_3d() const override;
        const std::vector<VkFramebuffer>& framebuffers_3d() const override;
        VkRenderPass render_pass_clear_3d() const override;
        const std::vector<VkFramebuffer>& framebuffers_clear_3d() const override;
        std::vector<VkClearValue> clear_values_3d(const Vector<3, float>& rgb) const override;

        VkRenderPass render_pass_2d() const override;
        const std::vector<VkFramebuffer>& framebuffers_2d() const override;

public:
        Impl(unsigned buffer_count,
             VkFormat color_format,
             const std::span<const VkFormat>& depth_formats,
             unsigned width,
             unsigned height,
             const std::vector<uint32_t>& family_indices,
             const vulkan::Device& device,
             int required_minimum_sample_count);

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
        const std::vector<uint32_t>& family_indices,
        const vulkan::Device& device,
        const int required_minimum_sample_count)
        : m_format(color_format), m_width(width), m_height(height)
{
        if (buffer_count < 1)
        {
                error("Buffer count " + std::to_string(buffer_count) + " must be positive");
        }

        if (width < 1 || height < 1)
        {
                error("Width " + std::to_string(width) + " and height " + std::to_string(height) + " must be positive");
        }

        create_buffers(
                device, depth_formats, buffer_count,
                vulkan::supported_color_depth_framebuffer_sample_count_flag(
                        device.physical_device(), required_minimum_sample_count),
                family_indices);

        render_buffer_check(m_color_attachments, m_depth_attachments);

        LOG(render_buffer_info(m_color_attachments, m_depth_attachments));
}

void Impl::create_buffers(
        const vulkan::Device& device,
        const std::span<const VkFormat>& depth_formats,
        const unsigned buffer_count,
        const VkSampleCountFlagBits sample_count,
        const std::vector<uint32_t>& family_indices)
{
        const std::vector<VkFormat> color_format = {m_format};

        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_color_attachments.emplace_back(
                        device, family_indices, color_format, sample_count, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(m_width, m_height),
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
                m_color_attachment_image_views.push_back(m_color_attachments.back().image_view());

                std::vector<VkFormat> formats;
                if (!m_depth_attachments.empty())
                {
                        formats = {m_depth_attachments[0].format()};
                }
                else
                {
                        formats = {depth_formats.begin(), depth_formats.end()};
                }
                m_depth_attachments.emplace_back(
                        device, family_indices, formats, sample_count, m_width, m_height,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        }

        const VkFormat depth_format = m_depth_attachments[0].format();

        std::vector<VkImageView> attachments;

        m_3d_render_pass = render_pass_color_depth(device, m_format, depth_format, sample_count, false);
        m_3d_render_pass_clear = render_pass_color_depth(device, m_format, depth_format, sample_count, true);

        attachments.resize(2);
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                attachments[0] = m_color_attachments[i].image_view();
                attachments[1] = m_depth_attachments[i].image_view();

                m_3d_framebuffers.push_back(
                        vulkan::create_framebuffer(device, m_3d_render_pass, m_width, m_height, attachments));
                m_3d_framebuffers_handles.push_back(m_3d_framebuffers.back());

                m_3d_framebuffers_clear.push_back(
                        vulkan::create_framebuffer(device, m_3d_render_pass_clear, m_width, m_height, attachments));
                m_3d_framebuffers_handles_clear.push_back(m_3d_framebuffers_clear.back());
        }

        m_2d_render_pass = render_pass_color(device, m_format, sample_count);

        attachments.resize(1);
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                attachments[0] = m_color_attachments[i].image_view();

                m_2d_framebuffers.push_back(
                        vulkan::create_framebuffer(device, m_2d_render_pass, m_width, m_height, attachments));
                m_2d_framebuffers_handles.push_back(m_2d_framebuffers.back());
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

VkRenderPass Impl::render_pass_3d() const
{
        return m_3d_render_pass;
}

const std::vector<VkFramebuffer>& Impl::framebuffers_3d() const
{
        ASSERT(!m_3d_framebuffers.empty() && m_3d_framebuffers.size() == m_3d_framebuffers_handles.size());
        return m_3d_framebuffers_handles;
}

VkRenderPass Impl::render_pass_clear_3d() const
{
        return m_3d_render_pass_clear;
}

const std::vector<VkFramebuffer>& Impl::framebuffers_clear_3d() const
{
        ASSERT(!m_3d_framebuffers_clear.empty()
               && m_3d_framebuffers_clear.size() == m_3d_framebuffers_handles_clear.size());
        return m_3d_framebuffers_handles_clear;
}

std::vector<VkClearValue> Impl::clear_values_3d(const Vector<3, float>& rgb) const
{
        std::vector<VkClearValue> clear_values(2);
        clear_values[0] = vulkan::create_color_clear_value(m_format, rgb);
        clear_values[1] = vulkan::create_depth_stencil_clear_value();
        return clear_values;
}

VkRenderPass Impl::render_pass_2d() const
{
        return m_2d_render_pass;
}

const std::vector<VkFramebuffer>& Impl::framebuffers_2d() const
{
        ASSERT(!m_2d_framebuffers.empty() && m_2d_framebuffers.size() == m_2d_framebuffers_handles.size());
        return m_2d_framebuffers_handles;
}

unsigned Impl::width() const
{
        return m_width;
}

unsigned Impl::height() const
{
        return m_height;
}

VkFormat Impl::color_format() const
{
        return m_format;
}

VkFormat Impl::depth_format() const
{
        ASSERT(!m_depth_attachments.empty());
        return m_depth_attachments[0].format();
}

VkSampleCountFlagBits Impl::sample_count() const
{
        return m_color_attachments[0].sample_count();
}

const std::vector<VkImageView>& Impl::image_views() const
{
        ASSERT(m_color_attachments.size() == m_color_attachment_image_views.size());
        return m_color_attachment_image_views;
}

void Impl::commands_color_resolve(
        VkCommandBuffer command_buffer,
        VkImage image,
        VkImageLayout layout,
        const Region<2, int>& rectangle,
        unsigned index) const
{
        ASSERT(index < m_color_attachments.size());
        ASSERT(m_color_attachments[index].sample_count() != VK_SAMPLE_COUNT_1_BIT);

        commands_image_resolve(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0,
                m_color_attachments[index].image(), COLOR_IMAGE_LAYOUT, image, layout, rectangle);
}

void Impl::commands_depth_copy(
        VkCommandBuffer command_buffer,
        VkImage image,
        VkImageLayout layout,
        const Region<2, int>& rectangle,
        unsigned index) const
{
        ASSERT(layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ASSERT(index < m_depth_attachments.size());

        commands_image_copy(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 0,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, m_depth_attachments[index].image(),
                DEPTH_IMAGE_LAYOUT, image, layout, rectangle);
}
}

std::unique_ptr<RenderBuffers> create_render_buffers(
        const unsigned buffer_count,
        const VkFormat color_format,
        const std::span<const VkFormat>& depth_formats,
        const unsigned width,
        const unsigned height,
        const std::vector<uint32_t>& family_indices,
        const vulkan::Device& device,
        const int required_minimum_sample_count)
{
        return std::make_unique<Impl>(
                buffer_count, color_format, depth_formats, width, height, family_indices, device,
                required_minimum_sample_count);
}
}