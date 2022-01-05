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

#include "compute.h"
#include "sampler.h"

#include "shaders/view.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/create.h>
#include <src/vulkan/features.h>
#include <src/vulkan/queue.h>

#include <thread>

namespace ns::gpu::pencil_sketch
{
namespace
{
constexpr int VERTEX_COUNT = 4;
constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R32_SFLOAT;

vulkan::DeviceFeatures device_features()
{
        vulkan::DeviceFeatures features{};
        features.features_10.vertexPipelineStoresAndAtomics = VK_TRUE;
        return features;
}

class Impl final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        // const bool sample_shading_;

        const vulkan::VulkanInstance* const instance_;
        const vulkan::Device* const device_;
        const vulkan::CommandPool* const graphics_command_pool_;
        const vulkan::Queue* const graphics_queue_;
        // const vulkan::CommandPool* const transfer_command_pool_;
        // const vulkan::Queue* const transfer_queue_;
        std::uint32_t graphics_family_index_;

        vulkan::handle::Semaphore signal_semaphore_;
        ViewProgram program_;
        ViewMemory memory_;
        std::unique_ptr<vulkan::BufferWithMemory> vertices_;
        vulkan::handle::Sampler sampler_;
        std::unique_ptr<vulkan::ImageWithMemory> image_;
        std::optional<vulkan::handle::Pipeline> pipeline_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_;

        std::unique_ptr<Compute> compute_;

        void draw_commands(const VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program_.pipeline_layout(),
                        ViewMemory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);

                const std::array<VkBuffer, 1> buffers{vertices_->buffer()};
                const std::array<VkDeviceSize, 1> offsets{0};
                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, VERTEX_COUNT, 1, 0, 0);
        }

        void create_buffers(
                RenderBuffers2D* const render_buffers,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& objects,
                const Region<2, int>& rectangle) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                image_ = std::make_unique<vulkan::ImageWithMemory>(
                        *device_, std::vector<std::uint32_t>({graphics_family_index_}),
                        std::vector<VkFormat>({IMAGE_FORMAT}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(rectangle.width(), rectangle.height()),
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, *graphics_command_pool_, *graphics_queue_);

                memory_.set_image(sampler_, image_->image_view());

                pipeline_ = program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), rectangle);

                compute_->create_buffers(sampler_, input, objects, rectangle, *image_);

                vulkan::CommandBufferCreateInfo info;
                info.device = *device_;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = render_buffers->width();
                info.render_area->extent.height = render_buffers->height();
                info.render_pass = render_buffers->render_pass();
                info.framebuffers = &render_buffers->framebuffers();
                info.command_pool = *graphics_command_pool_;
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

                ASSERT(queue.family_index() == graphics_family_index_);
                ASSERT(index < command_buffers_->count());

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (*command_buffers_)[index],
                        signal_semaphore_, queue);

                return signal_semaphore_;
        }

        void create_vertices()
        {
                std::array<ViewVertex, VERTEX_COUNT> vertices;

                // texture (0, 0) is top left
                vertices[0] = {{-1, +1, 0, 1}, {0, 1}};
                vertices[1] = {{+1, +1, 0, 1}, {1, 1}};
                vertices[2] = {{-1, -1, 0, 1}, {0, 0}};
                vertices[3] = {{+1, -1, 0, 1}, {1, 0}};

                vertices_.reset();
                vertices_ = std::make_unique<vulkan::BufferWithMemory>(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, *device_,
                        std::vector<std::uint32_t>(
                                {graphics_queue_->family_index() /*, transfer_queue_.family_index()*/}),
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(vertices));
                vertices_->write(
                        *graphics_command_pool_, *graphics_queue_, data_size(vertices), data_pointer(vertices));
        }

public:
        Impl(const vulkan::VulkanInstance* const instance,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue,
             const vulkan::CommandPool* const /*transfer_command_pool*/,
             const vulkan::Queue* const /*transfer_queue*/,
             const bool /*sample_shading*/)
                : // sample_shading_(sample_shading),
                  instance_(instance),
                  device_(&instance->device()),
                  graphics_command_pool_(graphics_command_pool),
                  graphics_queue_(graphics_queue),
                  // transfer_command_pool_(transfer_command_pool),
                  // transfer_queue_(transfer_queue),
                  graphics_family_index_(graphics_queue->family_index()),
                  signal_semaphore_(instance->device()),
                  program_(&instance->device()),
                  memory_(instance->device(), program_.descriptor_set_layout()),
                  sampler_(create_sampler(instance->device())),
                  compute_(create_compute(instance))
        {
                create_vertices();
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                instance_->device_wait_idle_noexcept("the Vulkan pencil sketch view destructor");
        }
};
}

vulkan::DeviceFeatures View::required_device_features()
{
        vulkan::DeviceFeatures features = device_features();
        vulkan::add_features(&features, Compute::required_device_features());
        return features;
}

std::unique_ptr<View> create_view(
        const vulkan::VulkanInstance* const instance,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const bool sample_shading)
{
        return std::make_unique<Impl>(
                instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading);
}
}
