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

#include "view.h"

#include "compute.h"
#include "function.h"
#include "option.h"
#include "sampler.h"

#include "shaders/view.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/merge.h>
#include <src/numerical/transform.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/create.h>
#include <src/vulkan/queue.h>

#include <thread>

namespace ns::gpu::optical_flow
{
namespace
{
// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::vertexPipelineStoresAndAtomics
};
// clang-format on

class Impl final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        // const bool sample_shading_;

        const vulkan::VulkanInstance& instance_;
        const vulkan::Device& device_;
        const vulkan::CommandPool& graphics_command_pool_;
        const vulkan::Queue& graphics_queue_;
        const vulkan::CommandPool& compute_command_pool_;
        // const vulkan::Queue& compute_queue_;
        //const vulkan::CommandPool& transfer_command_pool_;
        //const vulkan::Queue& transfer_queue_;

        vulkan::Semaphore signal_semaphore_;
        ViewProgram program_;
        ViewMemory memory_;
        vulkan::Sampler sampler_;
        std::optional<vulkan::BufferWithMemory> top_points_;
        std::optional<vulkan::BufferWithMemory> top_flow_;
        std::optional<vulkan::Pipeline> pipeline_points_;
        std::optional<vulkan::Pipeline> pipeline_lines_;
        std::optional<vulkan::CommandBuffers> command_buffers_;

        int top_point_count_;

        std::unique_ptr<Compute> compute_;

        void draw_commands(VkCommandBuffer command_buffer) const
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

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_points_);
                vkCmdDraw(command_buffer, top_point_count_ * 2, 1, 0, 0);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_lines_);
                vkCmdDraw(command_buffer, top_point_count_ * 2, 1, 0, 0);
        }

        void create_buffers(
                RenderBuffers2D* render_buffers,
                const vulkan::ImageWithMemory& input,
                double window_ppi,
                const Region<2, int>& rectangle) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                std::vector<vec2i> points;
                int point_count_x;
                int point_count_y;
                create_top_level_points(
                        rectangle.width(), rectangle.height(), DISTANCE_BETWEEN_POINTS_IN_MM, window_ppi,
                        &point_count_x, &point_count_y, &points);

                top_point_count_ = points.size();

                if (top_point_count_ == 0)
                {
                        return;
                }

                top_points_.emplace(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, device_,
                        std::vector<uint32_t>(
                                {graphics_command_pool_.family_index(), compute_command_pool_.family_index()}),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(points));
                top_points_->write(graphics_command_pool_, graphics_queue_, data_size(points), data_pointer(points));

                top_flow_.emplace(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, device_,
                        std::vector<uint32_t>(
                                {graphics_command_pool_.family_index(), compute_command_pool_.family_index()}),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, points.size() * sizeof(vec2f));

                pipeline_points_ = program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                        rectangle);
                pipeline_lines_ = program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                        rectangle);
                memory_.set_points(*top_points_);
                memory_.set_flow(*top_flow_);

                compute_->create_buffers(
                        sampler_, input, rectangle, point_count_x, point_count_y, *top_points_, *top_flow_);

                // (0, 0) is top left
                double left = 0;
                double right = rectangle.width();
                double bottom = rectangle.height();
                double top = 0;
                double near = 1;
                double far = -1;
                mat4d p = matrix::ortho_vulkan<double>(left, right, bottom, top, near, far);
                mat4d t = matrix::translate(vec3d(0.5, 0.5, 0));
                memory_.set_matrix(p * t);

                vulkan::CommandBufferCreateInfo info;
                info.device = device_;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = render_buffers->width();
                info.render_area->extent.height = render_buffers->height();
                info.render_pass = render_buffers->render_pass();
                info.framebuffers = &render_buffers->framebuffers();
                info.command_pool = graphics_command_pool_;
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
                unsigned index) override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                if (top_point_count_ == 0)
                {
                        return wait_semaphore;
                }

                //

                ASSERT(compute_queue.family_index() == compute_command_pool_.family_index());
                wait_semaphore = compute_->compute(compute_queue, wait_semaphore);

                //

                ASSERT(graphics_queue.family_index() == graphics_command_pool_.family_index());
                ASSERT(index < command_buffers_->count());

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, (*command_buffers_)[index],
                        signal_semaphore_, graphics_queue);

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
        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             const vulkan::CommandPool& compute_command_pool,
             const vulkan::Queue& compute_queue,
             const vulkan::CommandPool& /*transfer_command_pool*/,
             const vulkan::Queue& /*transfer_queue*/,
             bool /*sample_shading*/)
                : // sample_shading_(sample_shading),
                  instance_(instance),
                  device_(instance.device()),
                  graphics_command_pool_(graphics_command_pool),
                  graphics_queue_(graphics_queue),
                  compute_command_pool_(compute_command_pool),
                  // compute_queue_(compute_queue),
                  //transfer_command_pool_(transfer_command_pool),
                  //transfer_queue_(transfer_queue),
                  signal_semaphore_(instance.device()),
                  program_(instance.device()),
                  memory_(instance.device(), program_.descriptor_set_layout(), {graphics_queue.family_index()}),
                  sampler_(create_sampler(instance.device())),
                  compute_(create_compute(instance, compute_command_pool, compute_queue))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                instance_.device_wait_idle_noexcept("the Vulkan optical flow view destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> View::required_device_features()
{
        return merge<std::vector<vulkan::PhysicalDeviceFeatures>>(
                REQUIRED_DEVICE_FEATURES, Compute::required_device_features());
}

std::unique_ptr<View> create_view(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        bool sample_shading)
{
        return std::make_unique<Impl>(
                instance, graphics_command_pool, graphics_queue, compute_command_pool, compute_queue,
                transfer_command_pool, transfer_queue, sample_shading);
}
}
