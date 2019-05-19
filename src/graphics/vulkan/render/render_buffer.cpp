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

#include "render_buffer.h"

#include "command_buffer.h"
#include "render_pass.h"

#include "com/error.h"
#include "com/log.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/error.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/print.h"
#include "graphics/vulkan/query.h"
#include "graphics/vulkan/queue.h"

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
void check_buffers(const std::vector<vulkan::ColorAttachment>& color, const std::vector<vulkan::DepthAttachment>& depth)
{
        if (depth.empty())
        {
                error("No depth attachment");
        }

        if (!std::all_of(color.cbegin(), color.cend(),
                         [&](const vulkan::ColorAttachment& c) { return c.sample_count() == color[0].sample_count(); }))
        {
                error("Color attachments must have the same sample count");
        }

        if (!std::all_of(color.cbegin(), color.cend(),
                         [&](const vulkan::ColorAttachment& c) { return c.format() == color[0].format(); }))
        {
                error("Color attachments must have the same format");
        }

        if (!std::all_of(depth.cbegin(), depth.cend(),
                         [&](const vulkan::DepthAttachment& d) { return d.sample_count() == depth[0].sample_count(); }))
        {
                error("Depth attachments must have the same sample count");
        }

        if (!std::all_of(depth.cbegin(), depth.cend(),
                         [&](const vulkan::DepthAttachment& d) { return d.format() == depth[0].format(); }))
        {
                error("Depth attachments must have the same format");
        }

        if (!std::all_of(color.cbegin(), color.cend(),
                         [&](const vulkan::ColorAttachment& c) { return c.sample_count() == depth[0].sample_count(); }))
        {
                error("Color attachment sample count is not equal to depth attachment sample count");
        }

        if (color.size() == 0)
        {
                if (!std::all_of(depth.cbegin(), depth.cend(),
                                 [&](const vulkan::DepthAttachment& d) { return d.sample_count() == VK_SAMPLE_COUNT_1_BIT; }))
                {
                        error("There are no color attachments, but depth attachment sample count is not equal to 1");
                }
        }
}

std::string buffer_info(const std::vector<vulkan::ColorAttachment>& color, const std::vector<vulkan::DepthAttachment>& depth)
{
        check_buffers(color, depth);

        std::ostringstream oss;

        oss << "Render buffers sample count = "
            << vulkan::integer_sample_count_flag(color.size() > 0 ? color[0].sample_count() : VK_SAMPLE_COUNT_1_BIT);

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
        if (color.size() > 0)
        {
                oss << "Render buffers color attachment format = " << vulkan::format_to_string(color[0].format());
        }
        else
        {
                oss << "Render buffers do not have color attachments";
        }

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

        error_fatal("Command buffers not found");
}

unsigned compute_buffer_count(vulkan::RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain)
{
        switch (buffer_count)
        {
        case vulkan::RenderBufferCount::One:
                return 1;
        case vulkan::RenderBufferCount::Swapchain:
                ASSERT(swapchain.image_views().size() > 0);
                return swapchain.image_views().size();
        }
        error_fatal("Error render buffer count");
}

class Impl3D : public vulkan::RenderBuffers3D
{
        virtual std::vector<VkCommandBuffer> create_command_buffers_3d(
                const Color& clear_color,
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) = 0;

        virtual void delete_command_buffers_3d(std::vector<VkCommandBuffer>* buffers) = 0;

        virtual VkPipeline create_pipeline_3d(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                              const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                              const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                              const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) = 0;

        //

        std::vector<VkCommandBuffer> create_command_buffers(
                const Color& clear_color,
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) override final
        {
                return create_command_buffers_3d(clear_color, before_render_pass_commands, commands);
        }

        void delete_command_buffers(std::vector<VkCommandBuffer>* buffers) override final
        {
                delete_command_buffers_3d(buffers);
        }

        VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                   const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override final
        {
                return create_pipeline_3d(primitive_topology, sample_shading, shaders, pipeline_layout, vertex_binding,
                                          vertex_attribute);
        }

protected:
        ~Impl3D() override = default;
};

class Impl2D : public vulkan::RenderBuffers2D
{
        virtual std::vector<VkCommandBuffer> create_command_buffers_2d(
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) = 0;

        virtual void delete_command_buffers_2d(std::vector<VkCommandBuffer>* buffers) = 0;

        virtual VkPipeline create_pipeline_2d(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                              const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                              const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                              const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) = 0;

        //

        std::vector<VkCommandBuffer> create_command_buffers(
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) override final
        {
                return create_command_buffers_2d(before_render_pass_commands, commands);
        }

        void delete_command_buffers(std::vector<VkCommandBuffer>* buffers) override final
        {
                delete_command_buffers_2d(buffers);
        }

        VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                   const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                   const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                   const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override final
        {
                return create_pipeline_2d(primitive_topology, sample_shading, color_blend, shaders, pipeline_layout,
                                          vertex_binding, vertex_attribute);
        }

protected:
        ~Impl2D() override = default;
};

class Impl final : public vulkan::RenderBuffers, public Impl3D, public Impl2D
{
        const vulkan::Device& m_device;
        VkFormat m_swapchain_format;
        VkColorSpaceKHR m_swapchain_color_space;
        const vulkan::CommandPool& m_command_pool;

        //

        std::vector<vulkan::DepthAttachment> m_depth_attachments;
        std::vector<vulkan::ColorAttachment> m_color_attachments;

        vulkan::RenderPass m_render_pass_depth;
        vulkan::RenderPass m_render_pass;
        std::vector<vulkan::Framebuffer> m_framebuffers_depth;
        std::vector<vulkan::Framebuffer> m_framebuffers;

        std::list<vulkan::CommandBuffers> m_command_buffers_depth;
        std::list<vulkan::CommandBuffers> m_command_buffers;
        std::vector<vulkan::Pipeline> m_pipelines;

        vulkan::RenderPass m_resolve_render_pass;
        std::vector<vulkan::Framebuffer> m_resolve_framebuffers;
        std::vector<VkCommandBuffer> m_resolve_command_buffers;
        std::vector<vulkan::Semaphore> m_resolve_signal_semaphores;

        std::vector<vulkan::ColorAttachmentTexture> m_textures;
        vulkan::CommandBuffers m_textures_command_buffers;
        std::vector<vulkan::Semaphore> m_textures_signal_semaphores;

        void create_color_buffer_rendering(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                                           VkSampleCountFlagBits sample_count,
                                           const std::unordered_set<uint32_t>& attachment_family_indices);
#if 0
        void create_swapchain_rendering(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                                        const std::unordered_set<uint32_t>& attachment_family_indices,
                                        const std::vector<VkFormat>& depth_image_formats);
#endif

        void create_resolve_command_buffers();

        void create_textures(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                             const std::unordered_set<uint32_t>& family_indices, const vulkan::Queue& queue);

        //

        RenderBuffers3D& buffers_3d() override;
        RenderBuffers2D& buffers_2d() override;

        VkSemaphore resolve_to_swapchain(const vulkan::Queue& graphics_queue, VkSemaphore swapchain_image_semaphore,
                                         VkSemaphore wait_semaphore, unsigned image_index) const override;
        VkSemaphore resolve_to_texture(const vulkan::Queue& graphics_queue, VkSemaphore wait_semaphore,
                                       unsigned image_index) const override;

        //

        std::vector<VkCommandBuffer> create_command_buffers_3d(
                const Color& clear_color,
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) override;

        std::vector<VkCommandBuffer> create_command_buffers_2d(
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) override;

        //

        void delete_command_buffers_3d(std::vector<VkCommandBuffer>* buffers) override;

        void delete_command_buffers_2d(std::vector<VkCommandBuffer>* buffers) override;

        //

        VkPipeline create_pipeline_3d(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                      const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                      const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                      const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override;

        VkPipeline create_pipeline_2d(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                      const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                      const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                      const std::vector<VkVertexInputAttributeDescription>& vertex_attribute) override;

public:
        Impl(vulkan::RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain, const vulkan::CommandPool& command_pool,
             const vulkan::Queue& queue, const vulkan::Device& device, int required_minimum_sample_count);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};

Impl::Impl(vulkan::RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain, const vulkan::CommandPool& command_pool,
           const vulkan::Queue& queue, const vulkan::Device& device, int required_minimum_sample_count)
        : m_device(device),
          m_swapchain_format(swapchain.format()),
          m_swapchain_color_space(swapchain.color_space()),
          m_command_pool(command_pool)
{
        VkSampleCountFlagBits sample_count =
                vulkan::supported_framebuffer_sample_count_flag(m_device.physical_device(), required_minimum_sample_count);

        unsigned count = compute_buffer_count(buffer_count, swapchain);

#if 1
        create_color_buffer_rendering(count, swapchain, sample_count, {command_pool.family_index()});
        create_resolve_command_buffers();
        create_textures(count, swapchain, {command_pool.family_index()}, queue);
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

vulkan::RenderBuffers3D& Impl::buffers_3d()
{
        return *this;
}

vulkan::RenderBuffers2D& Impl::buffers_2d()
{
        return *this;
}

void Impl::create_color_buffer_rendering(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                                         VkSampleCountFlagBits sample_count,
                                         const std::unordered_set<uint32_t>& attachment_family_indices)
{
        namespace impl = vulkan_render_implementation;

        //

        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_color_attachments.emplace_back(m_device, attachment_family_indices, swapchain.format(), sample_count,
                                                 swapchain.width(), swapchain.height());

                std::vector<VkFormat> depth_formats;
                if (!m_depth_attachments.empty())
                {
                        depth_formats = {m_depth_attachments[0].format()};
                }
                else
                {
                        depth_formats = DEPTH_IMAGE_FORMATS;
                }

                m_depth_attachments.emplace_back(m_device, attachment_family_indices, depth_formats, sample_count,
                                                 swapchain.width(), swapchain.height());
        }

        const VkFormat depth_format = m_depth_attachments[0].format();

        ASSERT(m_depth_attachments.size() == 1 ||
               std::all_of(m_depth_attachments.cbegin(), m_depth_attachments.cend(),
                           [&](const vulkan::DepthAttachment& d) { return d.format() == depth_format; }));

        //

        std::vector<VkImageView> attachments;

        //

        m_render_pass_depth = impl::render_pass_color_depth(m_device, swapchain.format(), depth_format, sample_count);

        attachments.resize(2);
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                attachments[0] = m_color_attachments[i].image_view();
                attachments[1] = m_depth_attachments[i].image_view();

                m_framebuffers_depth.push_back(vulkan::create_framebuffer(m_device, m_render_pass_depth, swapchain.width(),
                                                                          swapchain.height(), attachments));
        }

        //

        m_render_pass = impl::render_pass_color(m_device, swapchain.format(), sample_count);

        attachments.resize(1);
        for (unsigned i = 0; i < buffer_count; ++i)
        {
                attachments[0] = m_color_attachments[i].image_view();

                m_framebuffers.push_back(
                        vulkan::create_framebuffer(m_device, m_render_pass, swapchain.width(), swapchain.height(), attachments));
        }

        //

        m_resolve_render_pass = impl::render_pass_swapchain_color(m_device, swapchain.format(), sample_count);

        attachments.resize(2);
        for (unsigned i = 0; i < swapchain.image_views().size(); ++i)
        {
                attachments[0] = swapchain.image_views()[i];
                attachments[1] = (buffer_count == 1) ? m_color_attachments[0].image_view() : m_color_attachments[i].image_view();

                m_resolve_framebuffers.push_back(vulkan::create_framebuffer(m_device, m_resolve_render_pass, swapchain.width(),
                                                                            swapchain.height(), attachments));
        }

        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_resolve_signal_semaphores.emplace_back(m_device);
        }
}

void Impl::create_textures(unsigned buffer_count, const vulkan::Swapchain& swapchain,
                           const std::unordered_set<uint32_t>& family_indices, const vulkan::Queue& queue)
{
        ASSERT(m_color_attachments.size() == buffer_count);

        for (unsigned i = 0; i < buffer_count; ++i)
        {
                m_textures.emplace_back(m_device, m_command_pool, queue, family_indices, swapchain.format(), swapchain.width(),
                                        swapchain.height());

                m_textures_signal_semaphores.emplace_back(m_device);
        }

        m_textures_command_buffers = vulkan::CommandBuffers(m_device, m_command_pool, buffer_count);

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkImageResolve image_resolve = {};
        image_resolve.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_resolve.srcSubresource.mipLevel = 0;
        image_resolve.srcSubresource.baseArrayLayer = 0;
        image_resolve.srcSubresource.layerCount = 1;
        image_resolve.srcOffset.x = 0;
        image_resolve.srcOffset.y = 0;
        image_resolve.srcOffset.z = 0;
        image_resolve.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_resolve.dstSubresource.mipLevel = 0;
        image_resolve.dstSubresource.baseArrayLayer = 0;
        image_resolve.dstSubresource.layerCount = 1;
        image_resolve.dstOffset.x = 0;
        image_resolve.dstOffset.y = 0;
        image_resolve.dstOffset.z = 0;
        image_resolve.extent.width = swapchain.width();
        image_resolve.extent.height = swapchain.height();
        image_resolve.extent.depth = 1;

        VkResult result;

        for (unsigned i = 0; i < buffer_count; ++i)
        {
                const VkCommandBuffer command_buffer = m_textures_command_buffers[i];

                result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                }

                //

                barrier.image = m_color_attachments[i].image();
                barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                     nullptr, 0, nullptr, 1, &barrier);

                barrier.image = m_textures[i].image();
                barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                     nullptr, 0, nullptr, 1, &barrier);

                //

                vkCmdResolveImage(command_buffer, m_color_attachments[i].image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  m_textures[i].image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_resolve);

                //

                barrier.image = m_color_attachments[i].image();
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = 0;

                vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                                     nullptr, 0, nullptr, 1, &barrier);

                barrier.image = m_textures[i].image();
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = 0;

                vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                                     nullptr, 0, nullptr, 1, &barrier);

                //

                result = vkEndCommandBuffer(command_buffer);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                }
        }
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

std::vector<VkCommandBuffer> Impl::create_command_buffers_3d(
        const Color& clear_color,
        const std::optional<std::function<void(VkCommandBuffer command_buffer)>>& before_render_pass_commands,
        const std::function<void(VkCommandBuffer buffer)>& commands)
{
        ASSERT(m_depth_attachments.size() > 0);

        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.render_pass = m_render_pass_depth;
        info.framebuffers.emplace(m_framebuffers_depth);
        info.command_pool = m_command_pool;
        info.before_render_pass_commands = before_render_pass_commands;
        info.render_pass_commands = commands;

        std::array<VkClearValue, 2> clear_values;
        clear_values[0] = vulkan::color_clear_value(m_swapchain_format, m_swapchain_color_space, clear_color);
        clear_values[1] = vulkan::depth_stencil_clear_value();
        info.clear_values.emplace(clear_values);

        m_command_buffers_depth.push_back(vulkan::create_command_buffers(info));

        return m_command_buffers_depth.back().buffers();
}

std::vector<VkCommandBuffer> Impl::create_command_buffers_2d(
        const std::optional<std::function<void(VkCommandBuffer command_buffer)>>& before_render_pass_commands,
        const std::function<void(VkCommandBuffer buffer)>& commands)
{
        ASSERT(m_depth_attachments.size() > 0);

        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.render_pass = m_render_pass;
        info.framebuffers.emplace(m_framebuffers);
        info.command_pool = m_command_pool;
        info.before_render_pass_commands = before_render_pass_commands;
        info.render_pass_commands = commands;

        m_command_buffers.push_back(vulkan::create_command_buffers(info));

        return m_command_buffers.back().buffers();
}

void Impl::create_resolve_command_buffers()
{
        ASSERT(m_depth_attachments.size() > 0);

        delete_buffers(&m_command_buffers, &m_resolve_command_buffers);

        if (m_color_attachments.empty())
        {
                return;
        }

        vulkan::CommandBufferCreateInfo info;
        info.device = m_device;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.render_pass = m_resolve_render_pass;
        info.framebuffers.emplace(m_resolve_framebuffers);
        info.command_pool = m_command_pool;

        m_command_buffers.push_back(vulkan::create_command_buffers(info));

        m_resolve_command_buffers = m_command_buffers.back().buffers();
}

VkSemaphore Impl::resolve_to_swapchain(const vulkan::Queue& graphics_queue, VkSemaphore swapchain_image_semaphore,
                                       VkSemaphore wait_semaphore, unsigned image_index) const
{
        ASSERT(graphics_queue.family_index() == m_command_pool.family_index());
        ASSERT(image_index < m_resolve_command_buffers.size());
        ASSERT(m_resolve_signal_semaphores.size() == 1 || image_index < m_resolve_signal_semaphores.size());

        const unsigned semaphore_index = m_resolve_signal_semaphores.size() == 1 ? 0 : image_index;

        std::array<VkSemaphore, 2> wait_semaphores;
        std::array<VkPipelineStageFlags, 2> wait_stages;

        wait_semaphores[0] = swapchain_image_semaphore;
        wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        wait_semaphores[1] = wait_semaphore;
        wait_stages[1] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        vulkan::queue_submit(wait_semaphores, wait_stages, m_resolve_command_buffers[image_index],
                             m_resolve_signal_semaphores[semaphore_index], graphics_queue);

        return m_resolve_signal_semaphores[semaphore_index];
}

VkSemaphore Impl::resolve_to_texture(const vulkan::Queue& graphics_queue, VkSemaphore wait_semaphore, unsigned image_index) const
{
        ASSERT(graphics_queue.family_index() == m_command_pool.family_index());
        ASSERT(m_textures.size() == 1 || image_index < m_textures.size());
        ASSERT(m_textures.size() == m_textures_command_buffers.count());
        ASSERT(m_textures.size() == m_textures_signal_semaphores.size());

        const unsigned index = m_textures.size() == 1 ? 0 : image_index;

        vulkan::queue_submit(wait_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_textures_command_buffers[index],
                             m_textures_signal_semaphores[index], graphics_queue);

        return m_textures_signal_semaphores[index];
}

void Impl::delete_command_buffers_3d(std::vector<VkCommandBuffer>* buffers)
{
        delete_buffers(&m_command_buffers_depth, buffers);
}

void Impl::delete_command_buffers_2d(std::vector<VkCommandBuffer>* buffers)
{
        delete_buffers(&m_command_buffers, buffers);
}

VkPipeline Impl::create_pipeline_3d(VkPrimitiveTopology primitive_topology, bool sample_shading,
                                    const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                    const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                    const std::vector<VkVertexInputAttributeDescription>& vertex_attribute)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);
        ASSERT(m_depth_attachments.size() > 0);

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = m_render_pass_depth;
        info.sub_pass = 0;
        info.sample_count = m_color_attachments.size() > 0 ? m_color_attachments[0].sample_count() : VK_SAMPLE_COUNT_1_BIT;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.primitive_topology = primitive_topology;
        info.shaders = &shaders;
        info.binding_descriptions = &vertex_binding;
        info.attribute_descriptions = &vertex_attribute;
        info.depth_bias = false;
        info.color_blend = false;

        m_pipelines.push_back(vulkan::create_graphics_pipeline(info));

        return m_pipelines.back();
}

VkPipeline Impl::create_pipeline_2d(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                    const std::vector<const vulkan::Shader*>& shaders, VkPipelineLayout pipeline_layout,
                                    const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                    const std::vector<VkVertexInputAttributeDescription>& vertex_attribute)
{
        ASSERT(pipeline_layout != VK_NULL_HANDLE);
        ASSERT(m_depth_attachments.size() > 0);

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = m_render_pass;
        info.sub_pass = 0;
        info.sample_count = m_color_attachments.size() > 0 ? m_color_attachments[0].sample_count() : VK_SAMPLE_COUNT_1_BIT;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout;
        info.width = m_depth_attachments[0].width();
        info.height = m_depth_attachments[0].height();
        info.primitive_topology = primitive_topology;
        info.shaders = &shaders;
        info.binding_descriptions = &vertex_binding;
        info.attribute_descriptions = &vertex_attribute;
        info.depth_bias = false;
        info.color_blend = color_blend;

        m_pipelines.push_back(vulkan::create_graphics_pipeline(info));

        return m_pipelines.back();
}
}

namespace vulkan
{
std::unique_ptr<RenderBuffers> create_render_buffers(RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain,
                                                     const vulkan::CommandPool& command_pool, const vulkan::Queue& queue,
                                                     const vulkan::Device& device, int required_minimum_sample_count)
{
        return std::make_unique<Impl>(buffer_count, swapchain, command_pool, queue, device, required_minimum_sample_count);
}
}
