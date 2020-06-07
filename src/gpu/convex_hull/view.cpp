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

#include "compute.h"
#include "size.h"

#include "shaders/view.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/merge.h>
#include <src/com/time.h>
#include <src/numerical/transform.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/create.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

#include <optional>
#include <thread>

namespace gpu::convex_hull
{
namespace
{
constexpr double ANGULAR_FREQUENCY = TWO_PI<double> * 5;

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::vertexPipelineStoresAndAtomics
};
// clang-format on

class Impl final : public View
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const bool m_sample_shading;
        double m_start_time;

        const uint32_t m_family_index;

        const vulkan::VulkanInstance& m_instance;
        VkCommandPool m_graphics_command_pool;

        vulkan::Semaphore m_semaphore;
        ViewProgram m_program;
        ViewMemory m_memory;
        std::optional<vulkan::BufferWithMemory> m_points;
        vulkan::BufferWithMemory m_indirect_buffer;
        std::optional<vulkan::Pipeline> m_pipeline;
        std::optional<vulkan::CommandBuffers> m_command_buffers;

        std::unique_ptr<Compute> m_compute;

        void reset_timer() override
        {
                m_start_time = time_in_seconds();
        }

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program.pipeline_layout(),
                        ViewMemory::set_number(), 1, &m_memory.descriptor_set(), 0, nullptr);

                ASSERT(m_indirect_buffer.usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
                vkCmdDrawIndirect(command_buffer, m_indirect_buffer, 0, 1, sizeof(VkDrawIndirectCommand));
        }

        void create_buffers(
                RenderBuffers2D* render_buffers,
                const vulkan::ImageWithMemory& objects,
                const Region<2, int>& rectangle) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_points.emplace(
                        vulkan::BufferMemoryType::DeviceLocal, m_instance.device(),
                        std::unordered_set({m_family_index}), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        points_buffer_size(rectangle.height()));

                m_memory.set_points(*m_points);

                // Матрица для рисования на плоскости окна, точка (0, 0) слева вверху
                double left = 0;
                double right = rectangle.width();
                double bottom = rectangle.height();
                double top = 0;
                double near = 1;
                double far = -1;
                mat4 p = matrix::ortho_vulkan<double>(left, right, bottom, top, near, far);
                mat4 t = matrix::translate(vec3(0.5, 0.5, 0));
                m_memory.set_matrix(p * t);

                m_pipeline = m_program.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, rectangle);

                m_compute->create_buffers(objects, rectangle, *m_points, m_indirect_buffer, m_family_index);

                vulkan::CommandBufferCreateInfo info;
                info.device = m_instance.device();
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
                m_points.reset();
        }

        VkSemaphore draw(const vulkan::Queue& queue, VkSemaphore wait_semaphore, unsigned image_index) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(queue.family_index() == m_family_index);

                float brightness = 0.5 + 0.5 * std::sin(ANGULAR_FREQUENCY * (time_in_seconds() - m_start_time));
                m_memory.set_brightness(brightness);

                //

                ASSERT(m_command_buffers->count() == 1 || image_index < m_command_buffers->count());

                const unsigned buffer_index = m_command_buffers->count() == 1 ? 0 : image_index;

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (*m_command_buffers)[buffer_index],
                        m_semaphore, queue);

                return m_semaphore;
        }

        static VkDrawIndirectCommand draw_indirect_command_data()
        {
                VkDrawIndirectCommand command = {};
                command.vertexCount = 0;
                command.instanceCount = 1;
                command.firstVertex = 0;
                command.firstInstance = 0;
                return command;
        }

public:
        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             bool sample_shading)
                : m_sample_shading(sample_shading),
                  m_family_index(graphics_command_pool.family_index()),
                  m_instance(instance),
                  m_graphics_command_pool(graphics_command_pool),
                  m_semaphore(instance.device()),
                  m_program(instance.device()),
                  m_memory(instance.device(), m_program.descriptor_set_layout(), {m_family_index}),
                  m_indirect_buffer(
                          vulkan::BufferMemoryType::DeviceLocal,
                          m_instance.device(),
                          {m_family_index},
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                          sizeof(VkDrawIndirectCommand)),
                  m_compute(create_compute(instance))
        {
                ASSERT(graphics_command_pool.family_index() == graphics_queue.family_index());
                const VkDrawIndirectCommand data = draw_indirect_command_data();
                m_indirect_buffer.write(graphics_command_pool, graphics_queue, data_size(data), data_pointer(data));
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan convex hull view destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> View::required_device_features()
{
        return merge<vulkan::PhysicalDeviceFeatures>(
                std::vector<vulkan::PhysicalDeviceFeatures>(REQUIRED_DEVICE_FEATURES),
                Compute::required_device_features());
}

std::unique_ptr<View> create_view(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        bool sample_shading)
{
        return std::make_unique<Impl>(instance, graphics_command_pool, graphics_queue, sample_shading);
}
}
