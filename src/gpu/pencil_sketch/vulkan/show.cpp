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

#include "show.h"

#include "compute.h"
#include "sampler.h"
#include "show_shader.h"

#include "com/container.h"
#include "com/error.h"
#include "com/merge.h"
#include "graphics/vulkan/commands.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/queue.h"

#include <thread>

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::vertexPipelineStoresAndAtomics
};
// clang-format on

constexpr int VERTEX_COUNT = 4;
constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R32_SFLOAT;

namespace gpu_vulkan
{
namespace
{
class Impl final : public PencilSketchShow
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        // const bool m_sample_shading;

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;
        const vulkan::CommandPool& m_graphics_command_pool;
        const vulkan::Queue& m_graphics_queue;
        const vulkan::CommandPool& m_transfer_command_pool;
        const vulkan::Queue& m_transfer_queue;
        uint32_t m_graphics_family_index;

        vulkan::Semaphore m_signal_semaphore;
        PencilSketchShowProgram m_program;
        PencilSketchShowMemory m_memory;
        std::unique_ptr<vulkan::BufferWithMemory> m_vertices;
        vulkan::Sampler m_sampler;
        std::unique_ptr<vulkan::ImageWithMemory> m_image;
        std::optional<vulkan::Pipeline> m_pipeline;
        std::optional<vulkan::CommandBuffers> m_command_buffers;

        std::unique_ptr<PencilSketchCompute> m_compute;

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program.pipeline_layout(),
                        PencilSketchShowMemory::set_number(), 1, &m_memory.descriptor_set(), 0, nullptr);

                const std::array<VkBuffer, 1> buffers{*m_vertices};
                const std::array<VkDeviceSize, 1> offsets{0};
                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, VERTEX_COUNT, 1, 0, 0);
        }

        void create_buffers(
                RenderBuffers2D* render_buffers,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& objects,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                const bool storage = true;
                m_image = std::make_unique<vulkan::ImageWithMemory>(
                        m_device, m_graphics_command_pool, m_graphics_queue,
                        std::unordered_set<uint32_t>({m_graphics_family_index}), std::vector<VkFormat>({IMAGE_FORMAT}),
                        width, height, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, storage);

                m_memory.set_image(m_sampler, *m_image);

                m_pipeline = m_program.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), x, y, width, height);

                m_compute->create_buffers(m_sampler, input, objects, x, y, width, height, *m_image);

                vulkan::CommandBufferCreateInfo info;
                info.device = m_device;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = render_buffers->width();
                info.render_area->extent.height = render_buffers->height();
                info.render_pass = render_buffers->render_pass();
                info.framebuffers = &render_buffers->framebuffers();
                info.command_pool = m_graphics_command_pool;
                info.before_render_pass_commands = [this](VkCommandBuffer command_buffer) {
                        m_compute->compute_commands(command_buffer);
                };
                info.render_pass_commands = [this](VkCommandBuffer command_buffer) { draw_commands(command_buffer); };
                m_command_buffers = vulkan::create_command_buffers(info);
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_command_buffers.reset();
                m_pipeline.reset();
                m_compute->delete_buffers();
                m_image.reset();
        }

        VkSemaphore draw(const vulkan::Queue& queue, VkSemaphore wait_semaphore, unsigned image_index) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(queue.family_index() == m_graphics_family_index);
                ASSERT(m_command_buffers->count() == 1 || image_index < m_command_buffers->count());

                const unsigned buffer_index = m_command_buffers->count() == 1 ? 0 : image_index;

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (*m_command_buffers)[buffer_index],
                        m_signal_semaphore, queue);

                return m_signal_semaphore;
        }

        void create_vertices()
        {
                std::array<PencilSketchShowVertex, VERTEX_COUNT> vertices;

                // Текстурный 0 находится вверху
                vertices[0] = {{-1, +1, 0, 1}, {0, 1}};
                vertices[1] = {{+1, +1, 0, 1}, {1, 1}};
                vertices[2] = {{-1, -1, 0, 1}, {0, 0}};
                vertices[3] = {{+1, -1, 0, 1}, {1, 0}};

                m_vertices.reset();
                m_vertices = std::make_unique<vulkan::BufferWithMemory>(
                        m_device, m_transfer_command_pool, m_transfer_queue,
                        std::unordered_set<uint32_t>(
                                {m_graphics_queue.family_index(), m_transfer_queue.family_index()}),
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data_size(vertices), vertices);

                ASSERT(m_vertices->usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
        }

public:
        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue,
             bool /*sample_shading*/)
                : // m_sample_shading(sample_shading),
                  m_instance(instance),
                  m_device(instance.device()),
                  m_graphics_command_pool(graphics_command_pool),
                  m_graphics_queue(graphics_queue),
                  m_transfer_command_pool(transfer_command_pool),
                  m_transfer_queue(transfer_queue),
                  m_graphics_family_index(graphics_queue.family_index()),
                  m_signal_semaphore(instance.device()),
                  m_program(instance.device()),
                  m_memory(instance.device(), m_program.descriptor_set_layout()),
                  m_sampler(create_pencil_sketch_sampler(instance.device())),
                  m_compute(create_pencil_sketch_compute(instance))
        {
                create_vertices();
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan pencil sketch show destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> PencilSketchShow::required_device_features()
{
        return merge<vulkan::PhysicalDeviceFeatures>(
                std::vector<vulkan::PhysicalDeviceFeatures>(REQUIRED_DEVICE_FEATURES),
                PencilSketchCompute::required_device_features());
}

std::unique_ptr<PencilSketchShow> create_convex_hull_show(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        bool sample_shading)
{
        return std::make_unique<Impl>(
                instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading);
}
}
