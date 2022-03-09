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

#include "view.h"

#include "sampler.h"

#include "shaders/view.h"

#include <src/com/container.h>
#include <src/numerical/transform.h>
#include <src/text/font.h>
#include <src/text/fonts.h>
#include <src/text/glyphs.h>
#include <src/text/vertices.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

#include <array>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ns::gpu::text_writer
{
namespace
{
constexpr int VERTEX_BUFFER_FIRST_SIZE = 10;

// clang-format off
constexpr std::array GRAYSCALE_IMAGE_FORMATS = std::to_array<VkFormat>
({
        VK_FORMAT_R8_SRGB,
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R32_SFLOAT
});
// clang-format on

std::vector<unsigned char> font_data()
{
        const text::Fonts& fonts = text::Fonts::instance();

        std::vector<std::string> font_names = fonts.names();
        if (font_names.empty())
        {
                error("Fonts not found");
        }

        return fonts.data(font_names.front());
}

class Glyphs
{
        image::Image<2> image_;
        std::unordered_map<char32_t, text::FontGlyph> glyphs_;

public:
        Glyphs(const int size, const unsigned max_image_dimension)
        {
                text::Font font(size, font_data());

                create_font_glyphs(font, max_image_dimension, max_image_dimension, &glyphs_, &image_);
        }

        std::unordered_map<char32_t, text::FontGlyph>& glyphs()
        {
                return glyphs_;
        }

        const image::Image<2>& image() const
        {
                return image_;
        }
};

class Impl final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const bool sample_shading_;

        const vulkan::Device* const device_;
        VkCommandPool graphics_command_pool_;

        vulkan::ImageWithMemory glyph_texture_;
        std::unordered_map<char32_t, text::FontGlyph> glyphs_;

        vulkan::handle::Semaphore semaphore_;
        vulkan::handle::Sampler sampler_;
        Program program_;
        Buffer buffer_;
        Memory memory_;
        std::optional<vulkan::BufferWithMemory> vertex_buffer_;
        vulkan::BufferWithMemory indirect_buffer_;
        RenderBuffers2D* render_buffers_ = nullptr;
        std::optional<vulkan::handle::Pipeline> pipeline_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_;

        std::uint32_t graphics_family_index_;

        void set_color(const color::Color& color) const override
        {
                buffer_.set_color(color.rgb32().clamp(0, 1));
        }

        void draw_commands(const VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                ASSERT(vertex_buffer_ && vertex_buffer_->buffer().size() > 0);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program_.pipeline_layout(),
                        Memory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);

                std::array<VkBuffer, 1> buffers = {vertex_buffer_->buffer()};
                std::array<VkDeviceSize, 1> offsets = {0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                ASSERT(indirect_buffer_.buffer().has_usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
                vkCmdDrawIndirect(command_buffer, indirect_buffer_.buffer(), 0, 1, sizeof(VkDrawIndirectCommand));
        }

        vulkan::handle::CommandBuffers create_commands()
        {
                vulkan::CommandBufferCreateInfo info;
                info.device = *device_;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = render_buffers_->width();
                info.render_area->extent.height = render_buffers_->height();
                info.render_pass = render_buffers_->render_pass();
                info.framebuffers = &render_buffers_->framebuffers();
                info.command_pool = graphics_command_pool_;
                info.render_pass_commands = [this](VkCommandBuffer command_buffer)
                {
                        draw_commands(command_buffer);
                };
                return vulkan::create_command_buffers(info);
        }

        void create_buffers(RenderBuffers2D* const render_buffers, const Region<2, int>& viewport) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                render_buffers_ = render_buffers;

                pipeline_ = program_.create_pipeline(
                        render_buffers_->render_pass(), render_buffers_->sample_count(), sample_shading_, viewport);

                command_buffers_ = create_commands();

                // (0, 0) is top left
                double left = 0;
                double right = viewport.width();
                double bottom = viewport.height();
                double top = 0;
                double near = 1;
                double far = -1;
                buffer_.set_matrix(matrix::ortho_vulkan<double>(left, right, bottom, top, near, far));
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                command_buffers_.reset();
                pipeline_.reset();
        }

        VkSemaphore draw(
                const vulkan::Queue& queue,
                const VkSemaphore wait_semaphore,
                const unsigned index,
                const text::TextData& text_data) override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                ASSERT(render_buffers_);
                ASSERT(queue.family_index() == graphics_family_index_);

                thread_local std::vector<text::TextVertex> vertices;

                text_vertices(glyphs_, text_data, &vertices);

                static_assert(sizeof(text::TextVertex) == sizeof(Vertex));
                static_assert(offsetof(text::TextVertex, window) == offsetof(Vertex, window_coordinates));
                static_assert(offsetof(text::TextVertex, texture) == offsetof(Vertex, texture_coordinates));
                static_assert(std::is_same_v<decltype(text::TextVertex::window), decltype(Vertex::window_coordinates)>);
                static_assert(
                        std::is_same_v<decltype(text::TextVertex::texture), decltype(Vertex::texture_coordinates)>);

                const std::size_t size = data_size(vertices);

                if (vertex_buffer_->buffer().size() < size)
                {
                        VULKAN_CHECK(vkQueueWaitIdle(queue));

                        command_buffers_.reset();

                        vertex_buffer_.emplace(
                                vulkan::BufferMemoryType::HOST_VISIBLE, *device_,
                                std::vector<std::uint32_t>({graphics_family_index_}), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                std::max(vertex_buffer_->buffer().size() * 2, size));

                        command_buffers_ = create_commands();
                }

                vulkan::map_and_write_to_buffer(*vertex_buffer_, vertices);

                VkDrawIndirectCommand command = {};
                command.vertexCount = vertices.size();
                command.instanceCount = 1;
                command.firstVertex = 0;
                command.firstInstance = 0;
                vulkan::map_and_write_to_buffer(indirect_buffer_, command);

                //

                ASSERT(index < command_buffers_->count());

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, (*command_buffers_)[index],
                        semaphore_, queue);

                return semaphore_;
        }

        Impl(const vulkan::Device* const device,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue,
             const vulkan::CommandPool* const /*transfer_command_pool*/,
             const vulkan::Queue* const /*transfer_queue*/,
             const bool sample_shading,
             const color::Color& color,
             Glyphs&& glyphs)
                : sample_shading_(sample_shading),
                  device_(device),
                  graphics_command_pool_(*graphics_command_pool),
                  glyph_texture_(
                          *device_,
                          std::vector<std::uint32_t>({graphics_queue->family_index()}),
                          std::vector<VkFormat>(
                                  std::cbegin(GRAYSCALE_IMAGE_FORMATS),
                                  std::cend(GRAYSCALE_IMAGE_FORMATS)),
                          VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_TYPE_2D,
                          vulkan::make_extent(glyphs.image().size[0], glyphs.image().size[1]),
                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          *graphics_command_pool,
                          *graphics_queue),
                  glyphs_(std::move(glyphs.glyphs())),
                  semaphore_(*device_),
                  sampler_(create_sampler(*device_)),
                  program_(device_),
                  buffer_(*device_, std::vector<std::uint32_t>({graphics_queue->family_index()})),
                  memory_(*device_,
                          program_.descriptor_set_layout(),
                          buffer_.buffer(),
                          sampler_,
                          glyph_texture_.image_view()),
                  vertex_buffer_(
                          std::in_place,
                          vulkan::BufferMemoryType::HOST_VISIBLE,
                          *device_,
                          std::vector<std::uint32_t>({graphics_queue->family_index()}),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VERTEX_BUFFER_FIRST_SIZE),
                  indirect_buffer_(
                          vulkan::BufferMemoryType::HOST_VISIBLE,
                          *device_,
                          std::vector<std::uint32_t>({graphics_queue->family_index()}),
                          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                          sizeof(VkDrawIndirectCommand)),
                  graphics_family_index_(graphics_queue->family_index())
        {
                glyph_texture_.write(
                        *graphics_command_pool, *graphics_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, glyphs.image().color_format, glyphs.image().pixels);

                set_color(color);
        }

public:
        Impl(const vulkan::Device* const device,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             const bool sample_shading,
             const int size,
             const color::Color& color)
                : Impl(device,
                       graphics_command_pool,
                       graphics_queue,
                       transfer_command_pool,
                       transfer_queue,
                       sample_shading,
                       color,
                       Glyphs(size, device->properties().properties_10.limits.maxImageDimension2D))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                device_->wait_idle_noexcept("text writer destructor");
        }
};
}

vulkan::DeviceFunctionality View::device_functionality()
{
        return {};
}

std::unique_ptr<View> create_view(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const bool sample_shading,
        const int size,
        const color::Color& color)
{
        return std::make_unique<Impl>(
                device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading,
                size, color);
}
}
