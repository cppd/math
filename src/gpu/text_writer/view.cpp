/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "glyphs.h"
#include "sampler.h"

#include "shaders/view.h"

#include <src/color/color.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/numerical/transform.h>
#include <src/text/text_data.h>
#include <src/text/vertices.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/error.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>
#include <src/vulkan/queue.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace ns::gpu::text_writer
{
namespace
{
constexpr int VERTEX_BUFFER_FIRST_SIZE = 10;

class Impl final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const bool sample_shading_;
        const vulkan::Device* const device_;
        const vulkan::CommandPool* const graphics_command_pool_;
        const vulkan::Queue* const graphics_queue_;
        const vulkan::handle::Semaphore semaphore_;
        const vulkan::handle::Sampler sampler_;
        const Program program_;
        const Buffer buffer_;
        const Memory memory_;
        const vulkan::BufferWithMemory indirect_buffer_;

        std::optional<vulkan::BufferWithMemory> vertex_buffer_;
        std::optional<vulkan::handle::Pipeline> pipeline_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_;
        std::optional<Glyphs> glyphs_;

        RenderBuffers2D* render_buffers_ = nullptr;

        void set_color(const color::Color& color) const override
        {
                buffer_.set_color(color.rgb32().clamp(0, 1));
        }

        void set_text_size(const unsigned size) override
        {
                if (glyphs_ && glyphs_->size() == size)
                {
                        return;
                }

                glyphs_.emplace(
                        size, *device_, *graphics_command_pool_, *graphics_queue_,
                        std::vector({graphics_queue_->family_index()}));

                memory_.set_image(sampler_, glyphs_->image_view());
        }

        void draw_commands(const VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                ASSERT(vertex_buffer_ && vertex_buffer_->buffer().size() > 0);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program_.pipeline_layout(),
                        Memory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);

                std::array<VkBuffer, 1> buffers = {vertex_buffer_->buffer().handle()};
                std::array<VkDeviceSize, 1> offsets = {0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                ASSERT(indirect_buffer_.buffer().has_usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
                vkCmdDrawIndirect(
                        command_buffer, indirect_buffer_.buffer().handle(), 0, 1, sizeof(VkDrawIndirectCommand));
        }

        vulkan::handle::CommandBuffers create_commands()
        {
                vulkan::CommandBufferCreateInfo info;
                info.device = device_->handle();
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = render_buffers_->width();
                info.render_area->extent.height = render_buffers_->height();
                info.render_pass = render_buffers_->render_pass().handle();
                info.framebuffers = &render_buffers_->framebuffers();
                info.command_pool = graphics_command_pool_->handle();
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
                const double left = 0;
                const double right = viewport.width();
                const double bottom = viewport.height();
                const double top = 0;
                const double near = 1;
                const double far = -1;
                buffer_.set_matrix(numerical::transform::ortho_vulkan<double>(left, right, bottom, top, near, far));
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
                ASSERT(queue.family_index() == graphics_queue_->family_index());
                ASSERT(glyphs_);

                thread_local std::vector<text::TextVertex> vertices;

                text::text_vertices(glyphs_->glyphs(), text_data, &vertices);

                static_assert(sizeof(text::TextVertex) == sizeof(Vertex));
                static_assert(offsetof(text::TextVertex, window) == offsetof(Vertex, window_coordinates));
                static_assert(offsetof(text::TextVertex, texture) == offsetof(Vertex, texture_coordinates));
                static_assert(std::is_same_v<decltype(text::TextVertex::window), decltype(Vertex::window_coordinates)>);
                static_assert(
                        std::is_same_v<decltype(text::TextVertex::texture), decltype(Vertex::texture_coordinates)>);

                const std::size_t size = data_size(vertices);

                if (vertex_buffer_->buffer().size() < size)
                {
                        VULKAN_CHECK(vkQueueWaitIdle(queue.handle()));

                        command_buffers_.reset();

                        vertex_buffer_.emplace(
                                vulkan::BufferMemoryType::HOST_VISIBLE, *device_,
                                std::vector({graphics_queue_->family_index()}), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
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
                        semaphore_, queue.handle());

                return semaphore_;
        }

public:
        Impl(const vulkan::Device* const device,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue,
             const bool sample_shading,
             const color::Color& color)
                : sample_shading_(sample_shading),
                  device_(device),
                  graphics_command_pool_(graphics_command_pool),
                  graphics_queue_(graphics_queue),
                  semaphore_(device_->handle()),
                  sampler_(create_sampler(device_->handle())),
                  program_(device_),
                  buffer_(*device_, {graphics_queue->family_index()}),
                  memory_(device_->handle(), program_.descriptor_set_layout(), buffer_.buffer()),
                  indirect_buffer_(
                          vulkan::BufferMemoryType::HOST_VISIBLE,
                          *device_,
                          {graphics_queue->family_index()},
                          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                          sizeof(VkDrawIndirectCommand)),
                  vertex_buffer_(
                          std::in_place,
                          vulkan::BufferMemoryType::HOST_VISIBLE,
                          *device_,
                          std::vector({graphics_queue->family_index()}),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VERTEX_BUFFER_FIRST_SIZE)
        {
                set_color(color);
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
        const bool sample_shading,
        const color::Color& color)
{
        return std::make_unique<Impl>(device, graphics_command_pool, graphics_queue, sample_shading, color);
}
}
