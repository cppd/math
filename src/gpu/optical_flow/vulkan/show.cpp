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

#include "compute.h"
#include "sampler.h"
#include "show_shader.h"

#include "com/container.h"
#include "com/error.h"
#include "com/matrix_alg.h"
#include "com/merge.h"
#include "gpu/optical_flow/com/show.h"
#include "graphics/vulkan/commands.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/queue.h"

#include <thread>

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::VertexPipelineStoresAndAtomics
};
// clang-format on

namespace gpu_vulkan
{
namespace
{
class Impl final : public OpticalFlowShow
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        // const bool m_sample_shading;

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;
        const vulkan::CommandPool& m_graphics_command_pool;
        // const vulkan::Queue& m_graphics_queue;
        const vulkan::CommandPool& m_compute_command_pool;
        // const vulkan::Queue& m_compute_queue;
        const vulkan::CommandPool& m_transfer_command_pool;
        const vulkan::Queue& m_transfer_queue;

        vulkan::Semaphore m_signal_semaphore;
        OpticalFlowShowProgram m_program;
        OpticalFlowShowMemory m_memory;
        vulkan::Sampler m_sampler;
        std::optional<vulkan::BufferWithMemory> m_top_points;
        std::optional<vulkan::BufferWithMemory> m_top_flow;
        std::optional<vulkan::Pipeline> m_pipeline_points;
        std::optional<vulkan::Pipeline> m_pipeline_lines;
        std::optional<vulkan::CommandBuffers> m_command_buffers;

        int m_top_point_count;

        std::unique_ptr<OpticalFlowCompute> m_compute;

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                if (m_top_point_count == 0)
                {
                        return;
                }

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program.pipeline_layout(),
                                        OpticalFlowShowMemory::set_number(), 1, &m_memory.descriptor_set(), 0, nullptr);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline_points);
                vkCmdDraw(command_buffer, m_top_point_count * 2, 1, 0, 0);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline_lines);
                vkCmdDraw(command_buffer, m_top_point_count * 2, 1, 0, 0);
        }

        void create_buffers(RenderBuffers2D* render_buffers, const vulkan::ImageWithMemory& input, double window_ppi, unsigned x,
                            unsigned y, unsigned width, unsigned height) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                std::vector<vec2i> points;
                int point_count_x, point_count_y;
                create_top_level_optical_flow_points(width, height, window_ppi, &point_count_x, &point_count_y, &points);

                m_top_point_count = points.size();

                if (m_top_point_count == 0)
                {
                        return;
                }

                m_top_points.emplace(m_device, m_transfer_command_pool, m_transfer_queue,
                                     std::unordered_set<uint32_t>({m_graphics_command_pool.family_index(),
                                                                   m_compute_command_pool.family_index(),
                                                                   m_transfer_command_pool.family_index()}),
                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, data_size(points), points);
                m_top_flow.emplace(vulkan::BufferMemoryType::DeviceLocal, m_device,
                                   std::unordered_set<uint32_t>(
                                           {m_graphics_command_pool.family_index(), m_compute_command_pool.family_index()}),
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, points.size() * sizeof(vec2f));

                m_pipeline_points = m_program.create_pipeline(render_buffers->render_pass(), render_buffers->sample_count(),
                                                              VK_PRIMITIVE_TOPOLOGY_POINT_LIST, x, y, width, height);
                m_pipeline_lines = m_program.create_pipeline(render_buffers->render_pass(), render_buffers->sample_count(),
                                                             VK_PRIMITIVE_TOPOLOGY_LINE_LIST, x, y, width, height);
                m_memory.set_points(*m_top_points);
                m_memory.set_flow(*m_top_flow);

                m_compute->create_buffers(m_sampler, input, x, y, width, height, point_count_x, point_count_y, *m_top_points,
                                          *m_top_flow);

                // Матрица для рисования на плоскости окна, точка (0, 0) слева вверху
                double left = 0;
                double right = width;
                double bottom = height;
                double top = 0;
                double near = 1;
                double far = -1;
                mat4 p = ortho_vulkan<double>(left, right, bottom, top, near, far);
                mat4 t = translate(vec3(0.5, 0.5, 0));
                m_memory.set_matrix(p * t);

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
                info.render_pass_commands = [this](VkCommandBuffer command_buffer) { draw_commands(command_buffer); };
                m_command_buffers = vulkan::create_command_buffers(info);
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_command_buffers.reset();
                m_pipeline_points.reset();
                m_pipeline_lines.reset();
                m_compute->delete_buffers();
                m_top_points.reset();
                m_top_flow.reset();
        }

        VkSemaphore draw(const vulkan::Queue& graphics_queue, const vulkan::Queue& compute_queue, VkSemaphore wait_semaphore,
                         unsigned image_index) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                if (m_top_point_count == 0)
                {
                        return wait_semaphore;
                }

                //

                ASSERT(compute_queue.family_index() == m_compute_command_pool.family_index());
                wait_semaphore = m_compute->compute(compute_queue, wait_semaphore);

                //

                ASSERT(graphics_queue.family_index() == m_graphics_command_pool.family_index());
                ASSERT(m_command_buffers->count() == 1 || image_index < m_command_buffers->count());

                const unsigned buffer_index = m_command_buffers->count() == 1 ? 0 : image_index;

                vulkan::queue_submit(wait_semaphore, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, (*m_command_buffers)[buffer_index],
                                     m_signal_semaphore, graphics_queue);

                return m_signal_semaphore;
        }

        void reset() override
        {
                if (m_top_point_count == 0)
                {
                        return;
                }

                m_compute->reset();
        }

public:
        Impl(const vulkan::VulkanInstance& instance, const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue, const vulkan::CommandPool& compute_command_pool,
             const vulkan::Queue& compute_queue, const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue, bool /*sample_shading*/)
                : // m_sample_shading(sample_shading),
                  m_instance(instance),
                  m_device(instance.device()),
                  m_graphics_command_pool(graphics_command_pool),
                  // m_graphics_queue(graphics_queue),
                  m_compute_command_pool(compute_command_pool),
                  // m_compute_queue(compute_queue),
                  m_transfer_command_pool(transfer_command_pool),
                  m_transfer_queue(transfer_queue),
                  m_signal_semaphore(instance.device()),
                  m_program(instance.device()),
                  m_memory(instance.device(), m_program.descriptor_set_layout(), {graphics_queue.family_index()}),
                  m_sampler(create_optical_flow_sampler(instance.device())),
                  m_compute(create_optical_flow_compute(instance, compute_command_pool, compute_queue, transfer_command_pool,
                                                        transfer_queue))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan optical flow show destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> OpticalFlowShow::required_device_features()
{
        return merge<vulkan::PhysicalDeviceFeatures>(std::vector<vulkan::PhysicalDeviceFeatures>(REQUIRED_DEVICE_FEATURES),
                                                     OpticalFlowCompute::required_device_features());
}

std::unique_ptr<OpticalFlowShow> create_optical_flow_show(
        const vulkan::VulkanInstance& instance, const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue, const vulkan::CommandPool& compute_command_pool, const vulkan::Queue& compute_queue,
        const vulkan::CommandPool& transfer_command_pool, const vulkan::Queue& transfer_queue, bool sample_shading)
{
        return std::make_unique<Impl>(instance, graphics_command_pool, graphics_queue, compute_command_pool, compute_queue,
                                      transfer_command_pool, transfer_queue, sample_shading);
}
}
