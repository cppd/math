/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "barrier.h"
#include "compute.h"
#include "size.h"

#include "shaders/view.h"

#include <src/com/chrono.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/numerical/transform.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/create.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

#include <optional>
#include <thread>

namespace ns::gpu::convex_hull
{
namespace
{
constexpr double ANGULAR_FREQUENCY = 5 * (2 * PI<double>);

vulkan::BufferWithMemory create_indirect_buffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue)
{
        ASSERT(graphics_command_pool.family_index() == graphics_queue.family_index());

        vulkan::BufferWithMemory buffer(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, {graphics_command_pool.family_index()},
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
                        | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                sizeof(VkDrawIndirectCommand));

        VkDrawIndirectCommand command = {};
        command.vertexCount = 0;
        command.instanceCount = 1;
        command.firstVertex = 0;
        command.firstInstance = 0;

        buffer.write(graphics_command_pool, graphics_queue, data_size(command), data_pointer(command));

        return buffer;
}

class Impl final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const bool sample_shading_;
        const std::uint32_t family_index_;
        const vulkan::Device* const device_;
        const VkCommandPool graphics_command_pool_;
        const vulkan::handle::Semaphore semaphore_;
        const ViewDataBuffer data_buffer_;
        const ViewProgram program_;
        const ViewMemory memory_;
        const vulkan::BufferWithMemory indirect_buffer_;

        std::optional<vulkan::BufferWithMemory> points_;
        std::optional<vulkan::handle::Pipeline> pipeline_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_;

        const std::unique_ptr<Compute> compute_;

        Clock::time_point start_time_ = Clock::now();

        void reset_timer() override
        {
                start_time_ = Clock::now();
        }

        void compute_commands(const VkCommandBuffer command_buffer) const
        {
                compute_->compute_commands(command_buffer);

                buffer_barrier(
                        command_buffer, points_->buffer().handle(), VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

                buffer_barrier(
                        command_buffer, indirect_buffer_.buffer().handle(), VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
        }

        void draw_commands(const VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program_.pipeline_layout(),
                        ViewMemory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);

                ASSERT(indirect_buffer_.buffer().has_usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
                vkCmdDrawIndirect(
                        command_buffer, indirect_buffer_.buffer().handle(), 0, 1, sizeof(VkDrawIndirectCommand));
        }

        void create_buffers(
                RenderBuffers2D* const render_buffers,
                const vulkan::ImageWithMemory& objects,
                const Region<2, int>& rectangle) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                points_.emplace(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, *device_, std::vector({family_index_}),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, points_buffer_size(rectangle.height()));

                memory_.set_points(points_->buffer());

                // (0, 0) is top left
                const double left = 0;
                const double right = rectangle.width();
                const double bottom = rectangle.height();
                const double top = 0;
                const double near = 1;
                const double far = -1;
                const Matrix4d p = numerical::transform::ortho_vulkan<double>(left, right, bottom, top, near, far);
                const Matrix4d t = numerical::transform::translate<double>(0.5, 0.5, 0);
                data_buffer_.set_matrix(p * t);

                pipeline_ = program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, rectangle);

                compute_->create_buffers(
                        objects, rectangle, points_->buffer(), indirect_buffer_.buffer(), family_index_);

                vulkan::CommandBufferCreateInfo info;
                info.device = device_->handle();
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = render_buffers->width();
                info.render_area->extent.height = render_buffers->height();
                info.render_pass = render_buffers->render_pass().handle();
                info.framebuffers = &render_buffers->framebuffers();
                info.command_pool = graphics_command_pool_;
                info.before_render_pass_commands = [this](const VkCommandBuffer command_buffer)
                {
                        compute_commands(command_buffer);
                };
                info.render_pass_commands = [this](const VkCommandBuffer command_buffer)
                {
                        draw_commands(command_buffer);
                };
                command_buffers_ = vulkan::create_command_buffers(info);
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                command_buffers_.reset();
                pipeline_.reset();
                compute_->delete_buffers();
                points_.reset();
        }

        VkSemaphore draw(const vulkan::Queue& queue, const VkSemaphore wait_semaphore, const unsigned index)
                const override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                const double brightness = 0.5 + 0.5 * std::sin(ANGULAR_FREQUENCY * duration_from(start_time_));
                data_buffer_.set_brightness(brightness);

                //

                ASSERT(queue.family_index() == family_index_);
                ASSERT(index < command_buffers_->count());

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (*command_buffers_)[index], semaphore_,
                        queue.handle());

                return semaphore_;
        }

public:
        Impl(const vulkan::Device* const device,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue,
             const bool sample_shading)
                : sample_shading_(sample_shading),
                  family_index_(graphics_command_pool->family_index()),
                  device_(device),
                  graphics_command_pool_(graphics_command_pool->handle()),
                  semaphore_(device_->handle()),
                  data_buffer_(*device_, {family_index_}),
                  program_(device_),
                  memory_(device_->handle(), program_.descriptor_set_layout(), data_buffer_.buffer()),
                  indirect_buffer_(create_indirect_buffer(*device_, *graphics_command_pool, *graphics_queue)),
                  compute_(create_compute(device_))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                device_->wait_idle_noexcept("convex hull view destructor");
        }
};
}

vulkan::DeviceFunctionality View::device_functionality()
{
        vulkan::DeviceFunctionality res;
        res.required_features.features_10.vertexPipelineStoresAndAtomics = VK_TRUE;
        res.required_features.features_13.maintenance4 = VK_TRUE;
        return res;
}

std::unique_ptr<View> create_view(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const bool sample_shading)
{
        return std::make_unique<Impl>(device, graphics_command_pool, graphics_queue, sample_shading);
}
}
