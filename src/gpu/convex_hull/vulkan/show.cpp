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
#include "shader_source.h"
#include "show_memory.h"

#include "com/container.h"
#include "com/error.h"
#include "com/log.h"
#include "com/merge.h"
#include "com/time.h"
#include "gpu/convex_hull/com/com.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/error.h"
#include "graphics/vulkan/queue.h"
#include "graphics/vulkan/shader.h"

#include <optional>
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
class Impl final : public ConvexHullShow
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const bool m_sample_shading;
        double m_start_time;

        const uint32_t m_family_index;

        const vulkan::VulkanInstance& m_instance;

        vulkan::Semaphore m_signal_semaphore;

        ConvexHullShaderMemory m_shader_memory;

        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

        vulkan::PipelineLayout m_pipeline_layout;

        std::optional<vulkan::BufferWithMemory> m_points;
        vulkan::BufferWithMemory m_indirect_buffer;

        vulkan::RenderBuffers2D* m_render_buffers = nullptr;
        std::vector<VkCommandBuffer> m_command_buffers;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        std::unique_ptr<gpu_vulkan::ConvexHullCompute> m_compute;

        void reset_timer() override
        {
                m_start_time = time_in_seconds();
        }

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
                                        m_shader_memory.set_number(), 1 /*set count*/, &m_shader_memory.descriptor_set(), 0,
                                        nullptr);

                ASSERT(m_indirect_buffer.usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
                vkCmdDrawIndirect(command_buffer, m_indirect_buffer, 0, 1, sizeof(VkDrawIndirectCommand));
        }

        void create_buffers(vulkan::RenderBuffers2D* render_buffers, const mat4& matrix,
                            const vulkan::StorageImage& objects) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_points.emplace(vulkan::BufferMemoryType::DeviceLocal, m_instance.device(), std::unordered_set({m_family_index}),
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, convex_hull_points_buffer_size(objects.height()));

                m_shader_memory.set_points(*m_points);
                m_shader_memory.set_matrix(matrix);

                m_render_buffers = render_buffers;

                m_pipeline = m_render_buffers->create_pipeline(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, m_sample_shading,
                                                               false /*color_blend*/, {&m_vertex_shader, &m_fragment_shader},
                                                               m_pipeline_layout, {}, {});

                m_compute->create_buffers(objects, *m_points, m_indirect_buffer, m_family_index);

                m_command_buffers = m_render_buffers->create_command_buffers(
                        [&](VkCommandBuffer command_buffer) { m_compute->compute_commands(command_buffer); },
                        std::bind(&Impl::draw_commands, this, std::placeholders::_1));
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_command_buffers.clear();

                m_compute->delete_buffers();
                m_points.reset();
        }

        VkSemaphore draw(const vulkan::Queue& queue, VkSemaphore wait_semaphore, unsigned image_index) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(m_render_buffers);
                ASSERT(queue.family_index() == m_family_index);

                float brightness = 0.5 + 0.5 * std::sin(CONVEX_HULL_ANGULAR_FREQUENCY * (time_in_seconds() - m_start_time));
                m_shader_memory.set_brightness(brightness);

                //

                ASSERT(m_command_buffers.size() == 1 || image_index < m_command_buffers.size());

                const unsigned buffer_index = m_command_buffers.size() == 1 ? 0 : image_index;

                vulkan::queue_submit(wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_command_buffers[buffer_index],
                                     m_signal_semaphore, queue);

                return m_signal_semaphore;
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
        Impl(const vulkan::VulkanInstance& instance, uint32_t family_index, bool sample_shading)
                : m_sample_shading(sample_shading),
                  m_family_index(family_index),
                  m_instance(instance),
                  m_signal_semaphore(instance.device()),
                  m_shader_memory(instance.device(), {m_family_index}),
                  m_vertex_shader(m_instance.device(), convex_hull_show_vert(), "main"),
                  m_fragment_shader(m_instance.device(), convex_hull_show_frag(), "main"),
                  m_pipeline_layout(vulkan::create_pipeline_layout(m_instance.device(), {m_shader_memory.set_number()},
                                                                   {m_shader_memory.descriptor_set_layout()})),
                  m_indirect_buffer(m_instance.device(), {m_family_index},
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                    draw_indirect_command_data()),
                  m_compute(gpu_vulkan::create_convex_hull_compute(instance))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan convex hull show destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> ConvexHullShow::required_device_features()
{
        return merge<vulkan::PhysicalDeviceFeatures>(std::vector<vulkan::PhysicalDeviceFeatures>(REQUIRED_DEVICE_FEATURES),
                                                     ConvexHullCompute::required_device_features());
}

std::unique_ptr<ConvexHullShow> create_convex_hull_show(const vulkan::VulkanInstance& instance, uint32_t family_index,
                                                        bool sample_shading)
{
        return std::make_unique<Impl>(instance, family_index, sample_shading);
}
}
