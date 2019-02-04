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

#include "ch_show.h"

#include "com/container.h"
#include "com/log.h"
#include "com/time.h"
#include "gpgpu/convex_hull/show/vulkan/shader/memory.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/error.h"
#include "graphics/vulkan/queue.h"
#include "graphics/vulkan/shader.h"

#include <optional>
#include <thread>

// Это в шейдерах layout(set = N, ...)
constexpr uint32_t SET_NUMBER = 0;

constexpr int INDIRECT_BUFFER_COMMAND_COUNT = 1;
constexpr int INDIRECT_BUFFER_COMMAND_NUMBER = 0;

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

namespace impl = gpgpu_vulkan::convex_hull_show_implementation;

namespace
{
constexpr std::array<vec2i, 4> TEST_POINTS = {vec2i(10, 10), vec2i(10, 1000), vec2i(1000, 1000), vec2i(10, 10)};

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

        vulkan::StorageBufferWithHostVisibleMemory m_points;
        vulkan::IndirectBufferWithHostVisibleMemory m_indirect_buffer;

        vulkan::RenderBuffers2D* m_render_buffers = nullptr;
        std::vector<VkCommandBuffer> m_command_buffers;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

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

                vkCmdDrawIndirect(command_buffer, m_indirect_buffer, m_indirect_buffer.offset(INDIRECT_BUFFER_COMMAND_NUMBER), 1,
                                  m_indirect_buffer.stride());
        }

        void create_buffers(vulkan::RenderBuffers2D* render_buffers, const mat4& matrix,
                            const vulkan::StorageImage& /*objects*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_shader_memory.set_points(m_points);
                m_shader_memory.set_matrix(matrix);

                m_render_buffers = render_buffers;

                m_pipeline = m_render_buffers->create_pipeline(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, m_sample_shading,
                                                               false /*color_blend*/, {&m_vertex_shader, &m_fragment_shader},
                                                               m_pipeline_layout, {}, {});

                m_command_buffers = m_render_buffers->create_command_buffers(
                        std::nullopt, std::bind(&Impl::draw_commands, this, std::placeholders::_1));
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_command_buffers.clear();
        }

        VkSemaphore draw(VkQueue graphics_queue, VkSemaphore wait_semaphore, unsigned image_index) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(m_render_buffers);

                float brightness = 0.5 + 0.5 * std::sin(ANGULAR_FREQUENCY * (time_in_seconds() - m_start_time));
                m_shader_memory.set_brightness(brightness);

                // int vertex_count = TEST_POINTS.size();
                int vertex_count = 0;

                m_indirect_buffer.set(INDIRECT_BUFFER_COMMAND_NUMBER, vertex_count, 1, 0, 0);

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
                  m_points(instance.device(), TEST_POINTS),
                  m_indirect_buffer(m_instance.device(), INDIRECT_BUFFER_COMMAND_COUNT)
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                try
                {
                        m_instance.device_wait_idle();
                }
                catch (std::exception& e)
                {
                        LOG(std::string("Device wait idle exception in the Vulkan convex hull show destructor: ") + e.what());
                }
                catch (...)
                {
                        LOG("Device wait idle unknown exception in the Vulkan convex hull destructor");
                }
        }
};
}

namespace gpgpu_vulkan
{
std::vector<vulkan::PhysicalDeviceFeatures> ConvexHullShow::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<ConvexHullShow> create_convex_hull_show(const vulkan::VulkanInstance& instance, bool sample_shading)
{
        return std::make_unique<Impl>(instance, sample_shading);
}
}
