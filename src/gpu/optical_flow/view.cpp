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

#include "compute.h"
#include "function.h"
#include "option.h"
#include "sampler.h"

#include "shaders/view.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>
#include <src/vulkan/queue.h>

#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ns::gpu::optical_flow
{
namespace
{
class Impl final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const vulkan::Device* const device_;
        const vulkan::CommandPool* const graphics_command_pool_;
        const vulkan::Queue* const graphics_queue_;
        const vulkan::CommandPool* const compute_command_pool_;
        const vulkan::handle::Semaphore signal_semaphore_;
        const ViewProgram program_;
        const ViewDataBuffer buffer_;
        const ViewMemory memory_;
        const vulkan::handle::Sampler sampler_;

        std::optional<vulkan::BufferWithMemory> top_points_;
        std::optional<vulkan::BufferWithMemory> top_flow_;
        std::optional<vulkan::handle::Pipeline> pipeline_points_;
        std::optional<vulkan::handle::Pipeline> pipeline_lines_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_;

        const std::unique_ptr<Compute> compute_;

        int top_point_count_;

        void draw_commands(const VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                if (top_point_count_ == 0)
                {
                        return;
                }

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program_.pipeline_layout(),
                        ViewMemory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);

                ASSERT(pipeline_points_);
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_points_);
                vkCmdDraw(command_buffer, top_point_count_ * 2, 1, 0, 0);

                ASSERT(pipeline_lines_);
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_lines_);
                vkCmdDraw(command_buffer, top_point_count_ * 2, 1, 0, 0);
        }

        void set_matrix(const numerical::Region<2, int>& rectangle) const
        {
                // (0, 0) is top left
                const double left = 0;
                const double right = rectangle.width();
                const double bottom = rectangle.height();
                const double top = 0;
                const double near = 1;
                const double far = -1;

                const numerical::Matrix4d p =
                        numerical::transform::ortho_vulkan<double>(left, right, bottom, top, near, far);
                const numerical::Matrix4d t = numerical::transform::translate<double>(0.5, 0.5, 0);

                buffer_.set_matrix(p * t);
        }

        void create_buffers(
                RenderBuffers2D* const render_buffers,
                const vulkan::ImageWithMemory& input,
                const double window_ppi,
                const numerical::Region<2, int>& rectangle) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                const TopLevelPoints top_level = create_top_level_points(
                        rectangle.width(), rectangle.height(), DISTANCE_BETWEEN_POINTS_IN_MM, window_ppi);

                top_point_count_ = top_level.points.size();

                if (top_point_count_ == 0)
                {
                        return;
                }

                top_points_.emplace(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, *device_,
                        std::vector({graphics_command_pool_->family_index(), compute_command_pool_->family_index()}),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        data_size(top_level.points));
                top_points_->write(
                        *graphics_command_pool_, *graphics_queue_, data_size(top_level.points),
                        data_pointer(top_level.points));

                top_flow_.emplace(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, *device_,
                        std::vector({graphics_command_pool_->family_index(), compute_command_pool_->family_index()}),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, top_level.points.size() * sizeof(numerical::Vector2f));

                pipeline_points_ = program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                        rectangle);
                pipeline_lines_ = program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                        rectangle);
                memory_.set_points(top_points_->buffer());
                memory_.set_flow(top_flow_->buffer());

                compute_->create_buffers(
                        sampler_, input, rectangle, top_level.count_x, top_level.count_y, top_points_->buffer(),
                        top_flow_->buffer());

                set_matrix(rectangle);

                vulkan::CommandBufferCreateInfo info;
                info.device = device_->handle();
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = render_buffers->width();
                info.render_area->extent.height = render_buffers->height();
                info.render_pass = render_buffers->render_pass().handle();
                info.framebuffers = &render_buffers->framebuffers();
                info.command_pool = graphics_command_pool_->handle();
                info.render_pass_commands = [this](VkCommandBuffer command_buffer)
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
                pipeline_points_.reset();
                pipeline_lines_.reset();
                compute_->delete_buffers();
                top_points_.reset();
                top_flow_.reset();
        }

        VkSemaphore draw(
                const vulkan::Queue& graphics_queue,
                const vulkan::Queue& compute_queue,
                VkSemaphore wait_semaphore,
                const unsigned index) const override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                if (top_point_count_ == 0)
                {
                        return wait_semaphore;
                }

                //

                ASSERT(compute_queue.family_index() == compute_command_pool_->family_index());
                wait_semaphore = compute_->compute(compute_queue, wait_semaphore);

                //

                ASSERT(graphics_queue.family_index() == graphics_command_pool_->family_index());
                ASSERT(command_buffers_);
                ASSERT(index < command_buffers_->count());

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, (*command_buffers_)[index],
                        signal_semaphore_, graphics_queue.handle());

                return signal_semaphore_;
        }

        void reset() override
        {
                if (top_point_count_ == 0)
                {
                        return;
                }

                compute_->reset();
        }

public:
        Impl(const vulkan::Device* const device,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue,
             const vulkan::CommandPool* const compute_command_pool,
             const vulkan::Queue* const compute_queue)
                : device_(device),
                  graphics_command_pool_(graphics_command_pool),
                  graphics_queue_(graphics_queue),
                  compute_command_pool_(compute_command_pool),
                  signal_semaphore_(device_->handle()),
                  program_(device_),
                  buffer_(*device_, {graphics_queue->family_index()}),
                  memory_(device_->handle(), program_.descriptor_set_layout(), buffer_.buffer()),
                  sampler_(create_sampler(device_->handle())),
                  compute_(create_compute(device_, compute_command_pool, compute_queue))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                device_->wait_idle_noexcept("optical flow view destructor");
        }
};
}

vulkan::physical_device::DeviceFunctionality View::device_functionality()
{
        vulkan::physical_device::DeviceFunctionality res;
        res.required_features.features_10.vertexPipelineStoresAndAtomics = VK_TRUE;
        res.required_features.features_13.maintenance4 = VK_TRUE;
        return res;
}

std::unique_ptr<View> create_view(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue)
{
        return std::make_unique<Impl>(
                device, graphics_command_pool, graphics_queue, compute_command_pool, compute_queue);
}
}
