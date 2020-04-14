/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "view.h"

#include "sampler.h"
#include "shader.h"

#include <src/com/container.h>
#include <src/com/merge.h>
#include <src/numerical/transform.h>
#include <src/text/font.h>
#include <src/text/glyphs.h>
#include <src/text/vertices.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>
#include <src/vulkan/sync.h>

#include <array>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gpu
{
namespace
{
constexpr int VERTEX_BUFFER_FIRST_SIZE = 10;

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
constexpr std::initializer_list<VkFormat> GRAYSCALE_IMAGE_FORMATS =
{
        VK_FORMAT_R8_SRGB,
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R32_SFLOAT
};
// clang-format on

class Glyphs
{
        int m_width;
        int m_height;
        std::unordered_map<char32_t, FontGlyph> m_glyphs;
        std::vector<std::uint_least8_t> m_pixels;

public:
        Glyphs(int size, unsigned max_image_dimension)
        {
                Font font(size);
                create_font_glyphs(
                        font, max_image_dimension, max_image_dimension, &m_glyphs, &m_width, &m_height, &m_pixels);
        }
        int width() const
        {
                return m_width;
        }
        int height() const
        {
                return m_height;
        }
        std::unordered_map<char32_t, FontGlyph>& glyphs()
        {
                return m_glyphs;
        }
        std::vector<std::uint_least8_t>& pixels()
        {
                return m_pixels;
        }
};

class Impl final : public TextView
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const bool m_sample_shading;

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;
        VkCommandPool m_graphics_command_pool;

        vulkan::ImageWithMemory m_glyph_texture;
        std::unordered_map<char32_t, FontGlyph> m_glyphs;

        vulkan::Semaphore m_semaphore;
        vulkan::Sampler m_sampler;
        TextProgram m_program;
        TextMemory m_memory;
        std::optional<vulkan::BufferWithMemory> m_vertex_buffer;
        vulkan::BufferWithMemory m_indirect_buffer;
        RenderBuffers2D* m_render_buffers = nullptr;
        std::optional<vulkan::Pipeline> m_pipeline;
        std::optional<vulkan::CommandBuffers> m_command_buffers;

        uint32_t m_graphics_family_index;

        void set_color(const Color& color) const override
        {
                m_memory.set_color(color);
        }

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                ASSERT(m_vertex_buffer && m_vertex_buffer->size() > 0);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program.pipeline_layout(),
                        TextMemory::set_number(), 1, &m_memory.descriptor_set(), 0, nullptr);

                std::array<VkBuffer, 1> buffers = {*m_vertex_buffer};
                std::array<VkDeviceSize, 1> offsets = {0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                ASSERT(m_indirect_buffer.usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
                vkCmdDrawIndirect(command_buffer, m_indirect_buffer, 0, 1, sizeof(VkDrawIndirectCommand));
        }

        vulkan::CommandBuffers create_commands()
        {
                vulkan::CommandBufferCreateInfo info;
                info.device = m_instance.device();
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = m_render_buffers->width();
                info.render_area->extent.height = m_render_buffers->height();
                info.render_pass = m_render_buffers->render_pass();
                info.framebuffers = &m_render_buffers->framebuffers();
                info.command_pool = m_graphics_command_pool;
                info.render_pass_commands = [this](VkCommandBuffer command_buffer) { draw_commands(command_buffer); };
                return vulkan::create_command_buffers(info);
        }

        void create_buffers(RenderBuffers2D* render_buffers, const Region<2, int>& viewport) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_render_buffers = render_buffers;

                m_pipeline = m_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(), m_sample_shading, viewport);

                m_command_buffers = create_commands();

                // Матрица для рисования на плоскости окна, точка (0, 0) слева вверху
                double left = 0;
                double right = viewport.width();
                double bottom = viewport.height();
                double top = 0;
                double near = 1;
                double far = -1;
                m_memory.set_matrix(matrix::ortho_vulkan<double>(left, right, bottom, top, near, far));
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_command_buffers.reset();
                m_pipeline.reset();
        }

        VkSemaphore draw(
                const vulkan::Queue& queue,
                VkSemaphore wait_semaphore,
                unsigned image_index,
                const TextData& text_data) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(m_render_buffers);
                ASSERT(queue.family_index() == m_graphics_family_index);

                thread_local std::vector<::TextVertex> vertices;

                text_vertices(m_glyphs, text_data, &vertices);

                static_assert(sizeof(::TextVertex) == sizeof(TextVertex));
                static_assert(offsetof(::TextVertex, v) == offsetof(TextVertex, window_coordinates));
                static_assert(offsetof(::TextVertex, t) == offsetof(TextVertex, texture_coordinates));
                static_assert(std::is_same_v<decltype(::TextVertex::v), decltype(TextVertex::window_coordinates)>);
                static_assert(std::is_same_v<decltype(::TextVertex::t), decltype(TextVertex::texture_coordinates)>);

                const size_t data_size = storage_size(vertices);

                if (m_vertex_buffer->size() < data_size)
                {
                        vulkan::queue_wait_idle(queue);

                        m_command_buffers.reset();

                        m_vertex_buffer.emplace(
                                vulkan::BufferMemoryType::HostVisible, m_device,
                                std::unordered_set({m_graphics_family_index}), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                std::max(m_vertex_buffer->size() * 2, data_size));

                        m_command_buffers = create_commands();
                }

                vulkan::map_and_write_to_buffer(*m_vertex_buffer, vertices);

                VkDrawIndirectCommand command = {};
                command.vertexCount = vertices.size();
                command.instanceCount = 1;
                command.firstVertex = 0;
                command.firstInstance = 0;
                vulkan::map_and_write_to_buffer(m_indirect_buffer, command);

                //

                ASSERT(m_command_buffers->count() == 1 || image_index < m_command_buffers->count());

                const unsigned buffer_index = m_command_buffers->count() == 1 ? 0 : image_index;

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        (*m_command_buffers)[buffer_index], m_semaphore, queue);

                return m_semaphore;
        }

        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue,
             bool sample_shading,
             const Color& color,
             Glyphs&& glyphs)
                : m_sample_shading(sample_shading),
                  m_instance(instance),
                  m_device(m_instance.device()),
                  m_graphics_command_pool(graphics_command_pool),
                  m_glyph_texture(
                          m_device,
                          graphics_command_pool,
                          graphics_queue,
                          transfer_command_pool,
                          transfer_queue,
                          std::unordered_set({graphics_queue.family_index(), transfer_queue.family_index()}),
                          GRAYSCALE_IMAGE_FORMATS,
                          glyphs.width(),
                          glyphs.height(),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          std::move(glyphs.pixels()),
                          false /*storage*/),
                  m_glyphs(std::move(glyphs.glyphs())),
                  m_semaphore(m_device),
                  m_sampler(create_text_sampler(m_device)),
                  m_program(m_device),
                  m_memory(
                          m_device,
                          m_program.descriptor_set_layout(),
                          std::unordered_set({graphics_queue.family_index()}),
                          m_sampler,
                          &m_glyph_texture),
                  m_vertex_buffer(
                          std::in_place,
                          vulkan::BufferMemoryType::HostVisible,
                          m_device,
                          std::unordered_set({graphics_queue.family_index()}),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VERTEX_BUFFER_FIRST_SIZE),
                  m_indirect_buffer(
                          vulkan::BufferMemoryType::HostVisible,
                          m_device,
                          std::unordered_set({graphics_queue.family_index()}),
                          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                          sizeof(VkDrawIndirectCommand)),
                  m_graphics_family_index(graphics_queue.family_index())
        {
                ASSERT(m_glyph_texture.usage() & VK_IMAGE_USAGE_SAMPLED_BIT);
                ASSERT(!(m_glyph_texture.usage() & VK_IMAGE_USAGE_STORAGE_BIT));

                set_color(color);
        }

public:
        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue,
             bool sample_shading,
             int size,
             const Color& color)
                : Impl(instance,
                       graphics_command_pool,
                       graphics_queue,
                       transfer_command_pool,
                       transfer_queue,
                       sample_shading,
                       color,
                       Glyphs(size, instance.limits().maxImageDimension2D))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan text destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> TextView::required_device_features()
{
        return merge<vulkan::PhysicalDeviceFeatures>(
                std::vector<vulkan::PhysicalDeviceFeatures>(REQUIRED_DEVICE_FEATURES));
}

std::unique_ptr<TextView> create_text_view(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        bool sample_shading,
        int size,
        const Color& color)
{
        return std::make_unique<Impl>(
                instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading,
                size, color);
}
}
