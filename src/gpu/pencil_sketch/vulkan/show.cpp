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
#include "shader_source.h"
#include "show_memory.h"
#include "show_vertex.h"

#include "com/container.h"
#include "com/error.h"
#include "com/merge.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/queue.h"
#include "graphics/vulkan/shader.h"

#include <thread>

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::VertexPipelineStoresAndAtomics
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

        PencilSketchShowMemory m_shader_memory;

        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Sampler m_sampler;

        std::unique_ptr<vulkan::BufferWithMemory> m_vertex_buffer;
        std::array<VkBuffer, 1> m_buffers;
        std::array<VkDeviceSize, 1> m_offsets;

        std::unique_ptr<vulkan::ImageWithMemory> m_image;

        std::vector<VkCommandBuffer> m_command_buffers;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        std::unique_ptr<PencilSketchCompute> m_compute;

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
                                        m_shader_memory.set_number(), 1 /*set count*/, &m_shader_memory.descriptor_set(), 0,
                                        nullptr);

                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());

                vkCmdDraw(command_buffer, VERTEX_COUNT, 1, 0, 0);
        }

        void create_buffers(RenderBuffers2D* render_buffers, const vulkan::ImageWithMemory& input,
                            const vulkan::ImageWithMemory& objects, unsigned x, unsigned y, unsigned width,
                            unsigned height) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                const bool storage = true;
                m_image = std::make_unique<vulkan::ImageWithMemory>(m_device, m_graphics_command_pool, m_graphics_queue,
                                                                    std::unordered_set<uint32_t>({m_graphics_family_index}),
                                                                    std::vector<VkFormat>({IMAGE_FORMAT}), width, height,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, storage);

                m_shader_memory.set_image(m_sampler, *m_image);

                m_pipeline = render_buffers->create_pipeline(
                        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, false /*sample_shading*/, false /*color_blend*/,
                        {&m_vertex_shader, &m_fragment_shader}, {nullptr, nullptr}, m_pipeline_layout,
                        PencilSketchShowVertex::binding_descriptions(), PencilSketchShowVertex::attribute_descriptions(), x, y,
                        width, height);

                m_compute->create_buffers(m_sampler, input, objects, x, y, width, height, *m_image);

                m_command_buffers = render_buffers->create_command_buffers(
                        [&](VkCommandBuffer command_buffer) { m_compute->compute_commands(command_buffer); },
                        std::bind(&Impl::draw_commands, this, std::placeholders::_1));
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_command_buffers.clear();
                m_pipeline = VK_NULL_HANDLE;
                m_compute->delete_buffers();
                m_image.reset();
        }

        VkSemaphore draw(const vulkan::Queue& queue, VkSemaphore wait_semaphore, unsigned image_index) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(queue.family_index() == m_graphics_family_index);
                ASSERT(m_command_buffers.size() == 1 || image_index < m_command_buffers.size());

                const unsigned buffer_index = m_command_buffers.size() == 1 ? 0 : image_index;

                vulkan::queue_submit(wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_command_buffers[buffer_index],
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

                m_vertex_buffer.reset();
                m_vertex_buffer = std::make_unique<vulkan::BufferWithMemory>(
                        m_device, m_transfer_command_pool, m_transfer_queue,
                        std::unordered_set<uint32_t>({m_graphics_queue.family_index(), m_transfer_queue.family_index()}),
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data_size(vertices), vertices);

                m_buffers = {*m_vertex_buffer};
                m_offsets = {0};

                ASSERT(m_vertex_buffer->usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
        }

public:
        Impl(const vulkan::VulkanInstance& instance, const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue, bool /*sample_shading*/)
                : // m_sample_shading(sample_shading),
                  m_instance(instance),
                  m_device(instance.device()),
                  m_graphics_command_pool(graphics_command_pool),
                  m_graphics_queue(graphics_queue),
                  m_transfer_command_pool(transfer_command_pool),
                  m_transfer_queue(transfer_queue),
                  m_graphics_family_index(graphics_queue.family_index()),
                  m_signal_semaphore(instance.device()),
                  m_shader_memory(instance.device()),
                  m_vertex_shader(m_device, pencil_sketch_show_vert(), "main"),
                  m_fragment_shader(m_device, pencil_sketch_show_frag(), "main"),
                  m_pipeline_layout(vulkan::create_pipeline_layout(m_device, {m_shader_memory.set_number()},
                                                                   {m_shader_memory.descriptor_set_layout()})),
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
        return merge<vulkan::PhysicalDeviceFeatures>(std::vector<vulkan::PhysicalDeviceFeatures>(REQUIRED_DEVICE_FEATURES),
                                                     PencilSketchCompute::required_device_features());
}

std::unique_ptr<PencilSketchShow> create_convex_hull_show(const vulkan::VulkanInstance& instance,
                                                          const vulkan::CommandPool& graphics_command_pool,
                                                          const vulkan::Queue& graphics_queue,
                                                          const vulkan::CommandPool& transfer_command_pool,
                                                          const vulkan::Queue& transfer_queue, bool sample_shading)
{
        return std::make_unique<Impl>(instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                                      sample_shading);
}
}
