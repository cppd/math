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

#include "vulkan_ch_show.h"

#include "com/container.h"
#include "com/error.h"
#include "com/log.h"
#include "com/merge.h"
#include "com/time.h"
#include "gpgpu/convex_hull/compute/vulkan_ch_compute.h"
#include "gpgpu/convex_hull/show/objects/vulkan_shader.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/error.h"
#include "graphics/vulkan/queue.h"
#include "graphics/vulkan/shader.h"

#include <optional>
#include <thread>

// Это в шейдерах layout(set = N, ...)
constexpr uint32_t SET_NUMBER = 0;

constexpr double ANGULAR_FREQUENCY = TWO_PI<double> * 5;

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::vertexPipelineStoresAndAtomics
};
// clang-format on

// clang-format off
constexpr uint32_t vertex_shader[]
{
#include "ch_show.vert.spr"
};
constexpr uint32_t fragment_shader[]
{
#include "ch_show.frag.spr"
};
// clang-format on

namespace impl = gpgpu_convex_hull_show_vulkan_implementation;

namespace
{
int points_buffer_size(int height)
{
        // 2 линии точек + 1 точка, тип ivec2
        return (2 * height + 1) * (2 * sizeof(int32_t));
}

class Impl final : public gpgpu_vulkan::ConvexHullShow
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const bool m_sample_shading;
        double m_start_time;

        const vulkan::VulkanInstance& m_instance;

        vulkan::Semaphore m_signal_semaphore;

        impl::ShaderMemory m_shader_memory;

        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

        vulkan::PipelineLayout m_pipeline_layout;

        std::optional<vulkan::BufferWithHostVisibleMemory> m_points;
        vulkan::BufferWithHostVisibleMemory m_indirect_buffer;

        vulkan::RenderBuffers2D* m_render_buffers = nullptr;
        std::vector<VkCommandBuffer> m_command_buffers;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        std::unique_ptr<gpgpu_vulkan::ConvexHullCompute> m_compute;

        void reset_timer() override
        {
                m_start_time = time_in_seconds();
        }

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

                VkDescriptorSet descriptor_set = m_shader_memory.descriptor_set();

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, SET_NUMBER,
                                        1 /*set count*/, &descriptor_set, 0, nullptr);

                ASSERT(m_indirect_buffer.usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
                vkCmdDrawIndirect(command_buffer, m_indirect_buffer, 0, 1, sizeof(VkDrawIndirectCommand));
        }

        void create_buffers(vulkan::RenderBuffers2D* render_buffers, const mat4& matrix,
                            const vulkan::StorageImage& objects) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_points.emplace(m_instance.device(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, points_buffer_size(objects.height()));

                m_shader_memory.set_points(*m_points);
                m_shader_memory.set_matrix(matrix);

                m_render_buffers = render_buffers;

                m_pipeline = m_render_buffers->create_pipeline(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, m_sample_shading,
                                                               false /*color_blend*/, {&m_vertex_shader, &m_fragment_shader},
                                                               m_pipeline_layout, {}, {});

                m_compute->create_buffers(objects, *m_points, m_indirect_buffer);

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

        VkSemaphore draw(VkQueue graphics_queue, VkSemaphore wait_semaphore, unsigned image_index) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(m_render_buffers);

                float brightness = 0.5 + 0.5 * std::sin(ANGULAR_FREQUENCY * (time_in_seconds() - m_start_time));
                m_shader_memory.set_brightness(brightness);

                //

                ASSERT(image_index < m_command_buffers.size());

                vulkan::queue_submit(wait_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     m_command_buffers[image_index], m_signal_semaphore, graphics_queue, VK_NULL_HANDLE);

                return m_signal_semaphore;
        }

public:
        Impl(const vulkan::VulkanInstance& instance, bool sample_shading)
                : m_sample_shading(sample_shading),
                  m_instance(instance),
                  m_signal_semaphore(instance.device()),
                  m_shader_memory(instance.device()),
                  m_vertex_shader(m_instance.device(), vertex_shader, "main"),
                  m_fragment_shader(m_instance.device(), fragment_shader, "main"),
                  m_pipeline_layout(vulkan::create_pipeline_layout(m_instance.device(), {SET_NUMBER},
                                                                   {m_shader_memory.descriptor_set_layout()})),
                  m_indirect_buffer(m_instance.device(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                    sizeof(VkDrawIndirectCommand)),
                  m_compute(gpgpu_vulkan::create_convex_hull_compute(instance))
        {
                VkDrawIndirectCommand command = {};
                command.vertexCount = 0;
                command.instanceCount = 1;
                command.firstVertex = 0;
                command.firstInstance = 0;
                m_indirect_buffer.write(0, command);
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan convex hull show destructor");
        }
};
}

namespace gpgpu_vulkan
{
std::vector<vulkan::PhysicalDeviceFeatures> ConvexHullShow::required_device_features()
{
        return merge<vulkan::PhysicalDeviceFeatures>(std::vector<vulkan::PhysicalDeviceFeatures>(REQUIRED_DEVICE_FEATURES),
                                                     ConvexHullCompute::required_device_features());
}

std::unique_ptr<ConvexHullShow> create_convex_hull_show(const vulkan::VulkanInstance& instance, bool sample_shading)
{
        return std::make_unique<Impl>(instance, sample_shading);
}
}
