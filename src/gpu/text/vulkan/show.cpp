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

#include "show.h"

#include "memory.h"
#include "sampler.h"
#include "shader_source.h"
#include "vertex.h"

#include "com/container.h"
#include "com/font/font.h"
#include "com/font/glyphs.h"
#include "com/font/vertices.h"
#include "com/log.h"
#include "com/matrix_alg.h"
#include "com/merge.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/error.h"
#include "graphics/vulkan/queue.h"
#include "graphics/vulkan/shader.h"
#include "graphics/vulkan/sync.h"

#include <array>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

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

namespace gpu_vulkan
{
namespace
{
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
                create_font_glyphs(font, max_image_dimension, max_image_dimension, &m_glyphs, &m_width, &m_height, &m_pixels);
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

class Impl final : public TextShow
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const bool m_sample_shading;

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;

        vulkan::Semaphore m_signal_semaphore;

        vulkan::Sampler m_sampler;
        vulkan::ImageWithMemory m_glyph_texture;
        std::unordered_map<char32_t, FontGlyph> m_glyphs;

        TextMemory m_shader_memory;

        vulkan::VertexShader m_text_vert;
        vulkan::FragmentShader m_text_frag;

        vulkan::PipelineLayout m_pipeline_layout;

        std::optional<vulkan::BufferWithMemory> m_vertex_buffer;
        vulkan::BufferWithMemory m_indirect_buffer;

        RenderBuffers2D* m_render_buffers = nullptr;
        std::vector<VkCommandBuffer> m_command_buffers;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        uint32_t m_graphics_family_index;

        void set_color(const Color& color) const override
        {
                m_shader_memory.set_color(color);
        }

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                ASSERT(m_vertex_buffer && m_vertex_buffer->size() > 0);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
                                        m_shader_memory.set_number(), 1 /*set count*/, &m_shader_memory.descriptor_set(), 0,
                                        nullptr);

                std::array<VkBuffer, 1> buffers = {*m_vertex_buffer};
                std::array<VkDeviceSize, 1> offsets = {0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                ASSERT(m_indirect_buffer.usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
                vkCmdDrawIndirect(command_buffer, m_indirect_buffer, 0, 1, sizeof(VkDrawIndirectCommand));
        }

        void create_buffers(RenderBuffers2D* render_buffers, unsigned x, unsigned y, unsigned width, unsigned height) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_render_buffers = render_buffers;

                m_pipeline = m_render_buffers->create_pipeline(
                        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, m_sample_shading, true /*color_blend*/, {&m_text_vert, &m_text_frag},
                        {nullptr, nullptr}, m_pipeline_layout, text_show_vertex_binding_descriptions(),
                        text_show_vertex_attribute_descriptions(), x, y, width, height);

                m_command_buffers = m_render_buffers->create_command_buffers(
                        std::nullopt, std::bind(&Impl::draw_commands, this, std::placeholders::_1));

                // Матрица для рисования на плоскости окна, точка (0, 0) слева вверху
                double left = 0;
                double right = width;
                double bottom = height;
                double top = 0;
                double near = 1;
                double far = -1;
                m_shader_memory.set_matrix(ortho_vulkan<double>(left, right, bottom, top, near, far));
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_command_buffers.clear();
        }

        VkSemaphore draw(const vulkan::Queue& queue, VkSemaphore wait_semaphore, unsigned image_index,
                         const TextData& text_data) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(m_render_buffers);
                ASSERT(queue.family_index() == m_graphics_family_index);

                thread_local std::vector<TextVertex> vertices;

                text_vertices(m_glyphs, text_data, &vertices);

                const size_t data_size = storage_size(vertices);

                if (m_vertex_buffer->size() < data_size)
                {
                        vulkan::queue_wait_idle(queue);

                        m_render_buffers->delete_command_buffers(&m_command_buffers);

                        m_vertex_buffer.emplace(vulkan::BufferMemoryType::HostVisible, m_device,
                                                std::unordered_set({m_graphics_family_index}), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                std::max(m_vertex_buffer->size() * 2, data_size));

                        m_command_buffers = m_render_buffers->create_command_buffers(
                                std::nullopt, std::bind(&Impl::draw_commands, this, std::placeholders::_1));
                }

                vulkan::map_and_write_to_buffer(*m_vertex_buffer, vertices);

                VkDrawIndirectCommand command = {};
                command.vertexCount = vertices.size();
                command.instanceCount = 1;
                command.firstVertex = 0;
                command.firstInstance = 0;
                vulkan::map_and_write_to_buffer(m_indirect_buffer, command);

                //

                ASSERT(m_command_buffers.size() == 1 || image_index < m_command_buffers.size());

                const unsigned buffer_index = m_command_buffers.size() == 1 ? 0 : image_index;

                vulkan::queue_submit(wait_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     m_command_buffers[buffer_index], m_signal_semaphore, queue);

                return m_signal_semaphore;
        }

        Impl(const vulkan::VulkanInstance& instance, const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue, bool sample_shading, const Color& color, Glyphs&& glyphs)
                : m_sample_shading(sample_shading),
                  m_instance(instance),
                  m_device(m_instance.device()),
                  m_signal_semaphore(m_device),
                  m_sampler(create_text_sampler(m_device)),
                  m_glyph_texture(m_device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                                  std::unordered_set({graphics_queue.family_index(), transfer_queue.family_index()}),
                                  GRAYSCALE_IMAGE_FORMATS, glyphs.width(), glyphs.height(),
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, std::move(glyphs.pixels()), false /*storage*/),
                  m_glyphs(std::move(glyphs.glyphs())),
                  m_shader_memory(m_device, std::unordered_set({graphics_queue.family_index()}), m_sampler, &m_glyph_texture),
                  m_text_vert(m_device, text_vert(), "main"),
                  m_text_frag(m_device, text_frag(), "main"),
                  m_pipeline_layout(vulkan::create_pipeline_layout(m_device, {m_shader_memory.set_number()},
                                                                   {m_shader_memory.descriptor_set_layout()})),
                  m_vertex_buffer(std::in_place, vulkan::BufferMemoryType::HostVisible, m_device,
                                  std::unordered_set({graphics_queue.family_index()}), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VERTEX_BUFFER_FIRST_SIZE),
                  m_indirect_buffer(vulkan::BufferMemoryType::HostVisible, m_device,
                                    std::unordered_set({graphics_queue.family_index()}), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                    sizeof(VkDrawIndirectCommand)),
                  m_graphics_family_index(graphics_queue.family_index())
        {
                ASSERT(m_glyph_texture.usage() & VK_IMAGE_USAGE_SAMPLED_BIT);
                ASSERT(!(m_glyph_texture.usage() & VK_IMAGE_USAGE_STORAGE_BIT));

                set_color(color);
        }

public:
        Impl(const vulkan::VulkanInstance& instance, const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue, bool sample_shading, int size, const Color& color)
                : Impl(instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading,
                       color, Glyphs(size, instance.limits().maxImageDimension2D))
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

std::vector<vulkan::PhysicalDeviceFeatures> TextShow::required_device_features()
{
        return merge<vulkan::PhysicalDeviceFeatures>(std::vector<vulkan::PhysicalDeviceFeatures>(REQUIRED_DEVICE_FEATURES));
}

std::unique_ptr<TextShow> create_text_show(const vulkan::VulkanInstance& instance,
                                           const vulkan::CommandPool& graphics_command_pool, const vulkan::Queue& graphics_queue,
                                           const vulkan::CommandPool& transfer_command_pool, const vulkan::Queue& transfer_queue,
                                           bool sample_shading, int size, const Color& color)
{
        return std::make_unique<Impl>(instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                                      sample_shading, size, color);
}
}
