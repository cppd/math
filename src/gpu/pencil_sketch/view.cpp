/*
Copyright (C) 2017-2026 Topological Manifold

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
#include "sampler.h"

#include "shaders/view.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>
#include <src/vulkan/queue.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ns::gpu::pencil_sketch
{
namespace
{
constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R32_SFLOAT;

// texture (0, 0) is top left
// clang-format off
constexpr std::array VERTICES = std::to_array<ViewVertex>
({
        {{-1,  1, 0, 1}, {0, 1}},
        {{ 1,  1, 0, 1}, {1, 1}},
        {{-1, -1, 0, 1}, {0, 0}},
        {{ 1, -1, 0, 1}, {1, 0}}
});
// clang-format on

static_assert(VERTICES.size() == 4);

vulkan::BufferWithMemory create_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue)
{
        vulkan::BufferWithMemory vertices(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device,
                std::vector({graphics_queue.family_index() /*, transfer_queue_.family_index()*/}),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(VERTICES));

        vertices.write(graphics_command_pool, graphics_queue, data_size(VERTICES), data_pointer(VERTICES));

        return vertices;
}

class Impl final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const vulkan::Device* const device_;
        const vulkan::CommandPool* const graphics_command_pool_;
        const vulkan::Queue* const graphics_queue_;
        const vulkan::handle::Semaphore signal_semaphore_;
        const ViewProgram program_;
        const ViewMemory memory_;
        const vulkan::handle::Sampler sampler_;
        const vulkan::BufferWithMemory vertices_;

        std::optional<vulkan::ImageWithMemory> image_;
        std::optional<vulkan::handle::Pipeline> pipeline_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_;

        const std::unique_ptr<Compute> compute_;

        void draw_commands(const VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                ASSERT(pipeline_);
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program_.pipeline_layout(),
                        ViewMemory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);

                const std::array<VkBuffer, 1> buffers{vertices_.buffer().handle()};
                const std::array<VkDeviceSize, 1> offsets{0};
                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, VERTICES.size(), 1, 0, 0);
        }

        void create_buffers(
                RenderBuffers2D* const render_buffers,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& objects,
                const numerical::Region<2, int>& rectangle) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                image_.emplace(
                        *device_, std::vector({graphics_queue_->family_index()}), std::vector({IMAGE_FORMAT}),
                        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(rectangle.width(), rectangle.height()),
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, *graphics_command_pool_, *graphics_queue_);

                memory_.set_image(sampler_, image_->image_view());

                pipeline_ = program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), rectangle);

                compute_->create_buffers(sampler_, input, objects, rectangle, *image_);

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
                info.before_render_pass_commands = [this](VkCommandBuffer command_buffer)
                {
                        compute_->compute_commands(command_buffer);
                };
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
                pipeline_.reset();
                compute_->delete_buffers();
                image_.reset();
        }

        VkSemaphore draw(const vulkan::Queue& queue, const VkSemaphore wait_semaphore, const unsigned index)
                const override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                ASSERT(queue.family_index() == graphics_queue_->family_index());
                ASSERT(command_buffers_);
                ASSERT(index < command_buffers_->count());

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (*command_buffers_)[index],
                        signal_semaphore_, queue.handle());

                return signal_semaphore_;
        }

public:
        Impl(const vulkan::Device* const device,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue)
                : device_(device),
                  graphics_command_pool_(graphics_command_pool),
                  graphics_queue_(graphics_queue),
                  signal_semaphore_(device_->handle()),
                  program_(device_),
                  memory_(device_->handle(), program_.descriptor_set_layout()),
                  sampler_(create_sampler(device_->handle())),
                  vertices_(create_vertices(*device_, *graphics_command_pool_, *graphics_queue_)),
                  compute_(create_compute(device_))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                device_->wait_idle_noexcept("pencil sketch view destructor");
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
        const vulkan::Queue* const graphics_queue)
{
        return std::make_unique<Impl>(device, graphics_command_pool, graphics_queue);
}
}
