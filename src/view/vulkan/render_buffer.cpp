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

#include "render_buffer.h"

#include "render_pass.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/copy.h>
#include <src/vulkan/create.h>
#include <src/vulkan/error.h>
#include <src/vulkan/print.h>
#include <src/vulkan/query.h>
#include <src/vulkan/queue.h>

#include <algorithm>
#include <sstream>

namespace ns::view
{
namespace
{
constexpr VkImageLayout COLOR_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
constexpr VkImageLayout DEPTH_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

// clang-format off
constexpr std::initializer_list<VkFormat> DEPTH_IMAGE_FORMATS =
{
        VK_FORMAT_D32_SFLOAT
};
// clang-format on

void check_buffers(
        const std::vector<vulkan::ColorAttachment>& color,
        const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        if (depth.empty())
        {
                error("No depth attachment");
        }

        if (!std::all_of(
                    color.cbegin(), color.cend(),
                    [&](const vulkan::ColorAttachment& c)
                    {
                            return c.sample_count() == color[0].sample_count();
                    }))
        {
                error("Color attachments must have the same sample count");
        }

        if (!std::all_of(
                    color.cbegin(), color.cend(),
                    [&](const vulkan::ColorAttachment& c)
                    {
                            return c.format() == color[0].format();
                    }))
        {
                error("Color attachments must have the same format");
        }

        if (!std::all_of(
                    depth.cbegin(), depth.cend(),
                    [&](const vulkan::DepthImageWithMemory& d)
                    {
                            return d.sample_count() == depth[0].sample_count();
                    }))
        {
                error("Depth attachments must have the same sample count");
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
                    color.cbegin(), color.cend(),
                    [&](const vulkan::ColorAttachment& c)
                    {
                            return c.sample_count() == depth[0].sample_count();
                    }))
        {
                error("Color attachment sample count is not equal to depth attachment sample count");
        }

        if (color.empty())
        {
                if (!std::all_of(
                            depth.cbegin(), depth.cend(),
                            [&](const vulkan::DepthImageWithMemory& d)
                            {
                                    return d.sample_count() == VK_SAMPLE_COUNT_1_BIT;
                            }))
                {
                        error("There are no color attachments, but depth attachment sample count is not equal to 1");
                }
        }
}

std::string buffer_info(
        const std::vector<vulkan::ColorAttachment>& color,
        const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        check_buffers(color, depth);

        std::ostringstream oss;

        oss << "Render buffers sample count = "
            << vulkan::integer_sample_count_flag(!color.empty() ? color[0].sample_count() : VK_SAMPLE_COUNT_1_BIT);

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
        if (!color.empty())
        {
                oss << "Render buffers color attachment format = " << vulkan::format_to_string(color[0].format());
        }
        else
        {
                oss << "Render buffers do not have color attachments";
        }

        return oss.str();
}

unsigned compute_buffer_count(RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain)
{
        switch (buffer_count)
        {
        case RenderBufferCount::One:
                return 1;
        case RenderBufferCount::Swapchain:
                ASSERT(!swapchain.image_views().empty());
                return swapchain.image_views().size();
        }
        error_fatal("Error render buffer count");
}

class Impl3D : public gpu::RenderBuffers3D
{
        virtual unsigned width_3d() const = 0;
        virtual unsigned height_3d() const = 0;
        virtual VkSampleCountFlagBits sample_count_3d() const = 0;

        virtual VkRenderPass render_pass_3d() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers_3d() const = 0;
        virtual VkRenderPass render_pass_clear_3d() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers_clear_3d() const = 0;

        virtual std::vector<VkClearValue> clear_values_3d(const Color& clear_color) const = 0;

        //

        unsigned width() const final
        {
                return width_3d();
        }

        unsigned height() const final
        {
                return height_3d();
        }

        VkSampleCountFlagBits sample_count() const final
        {
                return sample_count_3d();
        }

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

        std::vector<VkClearValue> clear_values(const Color& clear_color) const final
        {
                return clear_values_3d(clear_color);
        }

protected:
        ~Impl3D() override = default;
};

class Impl2D : public gpu::RenderBuffers2D
{
        virtual unsigned width_2d() const = 0;
        virtual unsigned height_2d() const = 0;
        virtual VkRenderPass render_pass_2d() const = 0;
        virtual VkSampleCountFlagBits sample_count_2d() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers_2d() const = 0;

        //

        unsigned width() const final
        {
                return width_2d();
        }

        unsigned height() const final
        {
                return height_2d();
        }

        VkSampleCountFlagBits sample_count() const final
        {
                return sample_count_2d();
        }

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
        const vulkan::Device& m_device;
        VkFormat m_swapchain_format;
        VkColorSpaceKHR m_swapchain_color_space;
        const vulkan::CommandPool& m_command_pool;

        const unsigned m_width, m_height;

        //

        std::vector<vulkan::DepthImageWithMemory> m_depth_attachments;
        std::vector<vulkan::ColorAttachment> m_color_attachments;

        //std::vector<VkClearValue> m_clear_values;

        vulkan::RenderPass m_3d_render_pass;
        vulkan::RenderPass m_3d_render_pass_clear;
        std::vector<vulkan::Framebuffer> m_3d_framebuffers;
        std::vector<vulkan::Framebuffer> m_3d_framebuffers_clear;
        std::vector<VkFramebuffer> m_3d_framebuffers_handles;
        std::vector<VkFramebuffer> m_3d_framebuffers_handles_clear;

        vulkan::RenderPass m_2d_render_pass;
        std::vector<vulkan::Framebuffer> m_2d_framebuffers;
        std::vector<VkFramebuffer> m_2d_framebuffers_handles;

        vulkan::RenderPass m_resolve_render_pass;
        std::vector<vulkan::Framebuffer> m_resolve_framebuffers;
        std::vector<VkFramebuffer> m_resolve_framebuffers_handles;
        vulkan::CommandBuffers m_resolve_command_buffers;
        std::vector<vulkan::Semaphore> m_resolve_signal_semaphores;

        void create_color_buffer_rendering(
                unsigned buffer_count,
                const vulkan::Swapchain& swapchain,
                VkSampleCountFlagBits sample_count,
                const std::unordered_set<uint32_t>& attachment_family_indices);
#if 0
        void create_swapchain_rendering(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                                        const std::unordered_set<uint32_t>& attachment_family_indices,
                                        const std::vector<VkFormat>& depth_image_formats);
#endif

        void create_resolve_command_buffers();

        //

        RenderBuffers3D& buffers_3d() override;
        RenderBuffers2D& buffers_2d() override;

        VkSemaphore resolve_to_swapchain(
                const vulkan::Queue& graphics_queue,
                VkSemaphore swapchain_image_semaphore,
                VkSemaphore wait_semaphore,
                unsigned image_index) const override;

        unsigned image_count() const override;

        void commands_color_resolve(
                VkCommandBuffer command_buffer,
                const vulkan::ImageWithMemory& image,
                VkImageLayout layout,
                const Region<2, int>& rectangle,
                unsigned image_index) const override;

        VkFormat depth_format() const override;
        void commands_depth_copy(
                VkCommandBuffer command_buffer,
                VkImage image,
                VkImageLayout layout,
                const Region<2, int>& rectangle,
                unsigned image_index) const override;

        //

        unsigned width_3d() const override;
        unsigned height_3d() const override;
        VkSampleCountFlagBits sample_count_3d() const override;

        VkRenderPass render_pass_3d() const override;
        const std::vector<VkFramebuffer>& framebuffers_3d() const override;

        VkRenderPass render_pass_clear_3d() const override;
        const std::vector<VkFramebuffer>& framebuffers_clear_3d() const override;

        std::vector<VkClearValue> clear_values_3d(const Color& clear_color) const override;

        //

        unsigned width_2d() const override;
        unsigned height_2d() const override;
        VkSampleCountFlagBits sample_count_2d() const override;

        VkRenderPass render_pass_2d() const override;
        const std::vector<VkFramebuffer>& framebuffers_2d() const override;

public:
        Impl(RenderBufferCount buffer_count,
             const vulkan::Swapchain& swapchain,
             const vulkan::CommandPool& command_pool,
             const vulkan::Device& device,
             int required_minimum_sample_count);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(
        RenderBufferCount buffer_count,
        const vulkan::Swapchain& swapchain,
        const vulkan::CommandPool& command_pool,
        const vulkan::Device& device,
        int required_minimum_sample_count)
        : m_device(device),
          m_swapchain_format(swapchain.format()),
          m_swapchain_color_space(swapchain.color_space()),
          m_command_pool(command_pool),
          m_width(swapchain.width()),
          m_height(swapchain.height())
{
        VkSampleCountFlagBits sample_count = vulkan::supported_framebuffer_sample_count_flag(
                m_device.physical_device(), required_minimum_sample_count);

        unsigned count = compute_buffer_count(buffer_count, swapchain);

#if 1
        create_color_buffer_rendering(count, swapchain, sample_count, {command_pool.family_index()});
        create_resolve_command_buffers();
#else
        if (sample_count != VK_SAMPLE_COUNT_1_BIT)
        {
                create_color_buffer_rendering(count, swapchain, sample_count, {command_pool.family_index()});
                create_resolve_command_buffers();
        }
        else
        {
                create_swapchain_rendering(count, swapchain, {command_pool.family_index()}, DEPTH_IMAGE_FORMATS);
        }
#endif

        check_buffers(m_color_attachments, m_depth_attachments);

        LOG(buffer_info(m_color_attachments, m_depth_attachments));
}

gpu::RenderBuffers3D& Impl::buffers_3d()
{
        return *this;
}

gpu::RenderBuffers2D& Impl::buffers_2d()
{
        return *this;
}

void Impl::create_color_buffer_rendering(
        unsigned buffer_count,
        const vulkan::Swapchain& swapchain,
        VkSampleCountFlagBits sample_count,
        const std::unordered_set<uint32_t>& attachment_family_indices)
{
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_color_attachments.emplace_back(
                        m_device, attachment_family_indices, swapchain.format(), sample_count, swapchain.width(),
                        swapchain.height());

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
                        m_device, attachment_family_indices, depth_formats, sample_count, swapchain.width(),
                        swapchain.height(),
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        }

        const VkFormat depth_format = m_depth_attachments[0].format();

        ASSERT(m_depth_attachments.size() == 1
               || std::all_of(
                       m_depth_attachments.cbegin(), m_depth_attachments.cend(),
                       [&](const vulkan::DepthImageWithMemory& d)
                       {
                               return d.format() == depth_format;
                       }));

        //

        std::vector<VkImageView> attachments;

        //

        m_3d_render_pass = render_pass_color_depth(m_device, swapchain.format(), depth_format, sample_count, false);
        m_3d_render_pass_clear =
                render_pass_color_depth(m_device, swapchain.format(), depth_format, sample_count, true);

        attachments.resize(2);
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                attachments[0] = m_color_attachments[i].image_view();
                attachments[1] = m_depth_attachments[i].image_view();

                m_3d_framebuffers.push_back(vulkan::create_framebuffer(
                        m_device, m_3d_render_pass, swapchain.width(), swapchain.height(), attachments));
                m_3d_framebuffers_handles.push_back(m_3d_framebuffers.back());

                m_3d_framebuffers_clear.push_back(vulkan::create_framebuffer(
                        m_device, m_3d_render_pass_clear, swapchain.width(), swapchain.height(), attachments));
                m_3d_framebuffers_handles_clear.push_back(m_3d_framebuffers_clear.back());
        }

        //

        m_2d_render_pass = render_pass_color(m_device, swapchain.format(), sample_count);

        attachments.resize(1);
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                attachments[0] = m_color_attachments[i].image_view();

                m_2d_framebuffers.push_back(vulkan::create_framebuffer(
                        m_device, m_2d_render_pass, swapchain.width(), swapchain.height(), attachments));
                m_2d_framebuffers_handles.push_back(m_2d_framebuffers.back());
        }

        //

        m_resolve_render_pass = render_pass_swapchain_color(m_device, swapchain.format(), sample_count);

        attachments.resize(2);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] =
                        (buffer_count == 1) ? m_color_attachments[0].image_view() : m_color_attachments[i].image_view();

                m_resolve_framebuffers.push_back(vulkan::create_framebuffer(
                        m_device, m_resolve_render_pass, swapchain.width(), swapchain.height(), attachments));
                m_resolve_framebuffers_handles.push_back(m_resolve_framebuffers.back());
        }

        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_resolve_signal_semaphores.emplace_back(m_device);
        }

        //m_clear_values.resize(2);
        //m_clear_values[1] = vulkan::depth_stencil_clear_value();
}

#if 0
void Impl::create_swapchain_rendering(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                                      const std::unordered_set<uint32_t>& attachment_family_indices,
                                      const std::vector<VkFormat>& depth_image_formats)
{
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_depth_attachments.emplace_back(m_device, attachment_family_indices, depth_image_formats, VK_SAMPLE_COUNT_1_BIT,
                                                 swapchain.width(), swapchain.height());
        }

        //

        namespace impl = vulkan_render_implementation;

        std::vector<VkImageView> attachments;

        m_render_pass_depth = impl::render_pass_swapchain_depth(m_device, swapchain.format(), m_depth_attachments[0].format());

        attachments.resize(2);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] = (buffer_count == 1) ? m_depth_attachments[0].image_view() : m_depth_attachments[i].image_view();

                m_framebuffers_depth.push_back(
                        create_framebuffer(m_device, m_render_pass_depth, swapchain.width(), swapchain.height(), attachments));
        }

        //

        m_render_pass = impl::render_pass_swapchain(m_device, swapchain.format());

        attachments.resize(1);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];

                m_framebuffers.push_back(
                        create_framebuffer(m_device, m_render_pass, swapchain.width(), swapchain.height(), attachments));
        }
}
#endif

unsigned Impl::width_3d() const
{
        ASSERT(!m_depth_attachments.empty());
        return m_depth_attachments[0].width();
}

unsigned Impl::height_3d() const
{
        ASSERT(!m_depth_attachments.empty());
        return m_depth_attachments[0].height();
}

VkSampleCountFlagBits Impl::sample_count_3d() const
{
        return !m_color_attachments.empty() ? m_color_attachments[0].sample_count() : VK_SAMPLE_COUNT_1_BIT;
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

std::vector<VkClearValue> Impl::clear_values_3d(const Color& clear_color) const
{
        std::vector<VkClearValue> clear_values(2);
        clear_values[0] = vulkan::color_clear_value(m_swapchain_format, m_swapchain_color_space, clear_color);
        clear_values[1] = vulkan::depth_stencil_clear_value();
        return clear_values;
}

unsigned Impl::width_2d() const
{
        ASSERT(!m_depth_attachments.empty());
        return m_depth_attachments[0].width();
}

unsigned Impl::height_2d() const
{
        ASSERT(!m_depth_attachments.empty());
        return m_depth_attachments[0].height();
}

VkSampleCountFlagBits Impl::sample_count_2d() const
{
        return !m_color_attachments.empty() ? m_color_attachments[0].sample_count() : VK_SAMPLE_COUNT_1_BIT;
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

void Impl::create_resolve_command_buffers()
{
        ASSERT(!m_depth_attachments.empty());

        m_resolve_command_buffers = vulkan::CommandBuffers();

        if (m_color_attachments.empty())
        {
                return;
        }

        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = m_depth_attachments[0].width();
        info.render_area->extent.height = m_depth_attachments[0].height();
        info.render_pass = m_resolve_render_pass;
        info.framebuffers = &m_resolve_framebuffers_handles;
        info.command_pool = m_command_pool;

        m_resolve_command_buffers = vulkan::create_command_buffers(info);
}

VkSemaphore Impl::resolve_to_swapchain(
        const vulkan::Queue& graphics_queue,
        VkSemaphore swapchain_image_semaphore,
        VkSemaphore wait_semaphore,
        unsigned image_index) const
{
        ASSERT(graphics_queue.family_index() == m_command_pool.family_index());
        ASSERT(image_index < m_resolve_command_buffers.count());
        ASSERT(m_resolve_signal_semaphores.size() == 1 || image_index < m_resolve_signal_semaphores.size());

        const unsigned semaphore_index = m_resolve_signal_semaphores.size() == 1 ? 0 : image_index;

        std::array<VkSemaphore, 2> wait_semaphores;
        std::array<VkPipelineStageFlags, 2> wait_stages;

        wait_semaphores[0] = swapchain_image_semaphore;
        wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        wait_semaphores[1] = wait_semaphore;
        wait_stages[1] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        vulkan::queue_submit(
                wait_semaphores, wait_stages, m_resolve_command_buffers[image_index],
                m_resolve_signal_semaphores[semaphore_index], graphics_queue);

        return m_resolve_signal_semaphores[semaphore_index];
}

unsigned Impl::image_count() const
{
        return m_color_attachments.size();
}

void Impl::commands_color_resolve(
        VkCommandBuffer command_buffer,
        const vulkan::ImageWithMemory& image,
        VkImageLayout layout,
        const Region<2, int>& rectangle,
        unsigned image_index) const
{
        ASSERT(image_index < m_color_attachments.size());
        ASSERT(m_color_attachments[image_index].sample_count() != VK_SAMPLE_COUNT_1_BIT);
        ASSERT(image.sample_count() == VK_SAMPLE_COUNT_1_BIT);

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VkResult result;

        result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
        }

        vulkan::commands_image_resolve(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0,
                m_color_attachments[image_index].image(), COLOR_IMAGE_LAYOUT, image.image(), layout, rectangle);

        result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
        }
}

VkFormat Impl::depth_format() const
{
        ASSERT(!m_depth_attachments.empty());
        return m_depth_attachments[0].format();
}

void Impl::commands_depth_copy(
        VkCommandBuffer command_buffer,
        VkImage image,
        VkImageLayout layout,
        const Region<2, int>& rectangle,
        unsigned image_index) const
{
        ASSERT(layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ASSERT(image_index < m_depth_attachments.size());

        vulkan::commands_image_copy(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 0,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, m_depth_attachments[image_index].image(),
                DEPTH_IMAGE_LAYOUT, image, layout, rectangle);
}
}

std::unique_ptr<RenderBuffers> create_render_buffers(
        RenderBufferCount buffer_count,
        const vulkan::Swapchain& swapchain,
        const vulkan::CommandPool& command_pool,
        const vulkan::Device& device,
        int required_minimum_sample_count)
{
        return std::make_unique<Impl>(buffer_count, swapchain, command_pool, device, required_minimum_sample_count);
}
}
