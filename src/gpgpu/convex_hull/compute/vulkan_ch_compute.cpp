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

#include "vulkan_ch_compute.h"

#include "com/log.h"
#include "gpgpu/convex_hull/compute/objects/vulkan_memory.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/shader.h"

#include <thread>

constexpr uint32_t SET_NUMBER = 0;

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

// clang-format off
constexpr uint32_t compute_shader[]
{
#include "ch_debug.comp.spr"
};
// clang-format on

namespace impl = gpgpu_vulkan::convex_hull_compute_implementation;

namespace
{
class Impl final : public gpgpu_vulkan::ConvexHullCompute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;

        impl::ShaderMemory m_shader_memory;

        vulkan::ComputeShader m_compute_shader;

        vulkan::PipelineLayout m_pipeline_layout;

        vulkan::Pipeline m_pipeline;
        VkBuffer m_points_buffer = VK_NULL_HANDLE;

        void compute_commands(VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(m_points_buffer != VK_NULL_HANDLE);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

                VkDescriptorSet descriptor_set = m_shader_memory.descriptor_set();

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, SET_NUMBER,
                                        1 /*set count*/, &descriptor_set, 0, nullptr);

                vkCmdDispatch(command_buffer, 1, 1, 1);

                VkBufferMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.buffer = m_points_buffer;
                barrier.offset = 0;
                barrier.size = VK_WHOLE_SIZE;
                vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                     VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &barrier, 0, nullptr);
        }

        void create_buffers(const vulkan::StorageImage& objects,
                            const vulkan::StorageBufferWithHostVisibleMemory& points) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_shader_memory.set_object_image(objects);
                m_shader_memory.set_points(points);

                vulkan::ComputePipelineCreateInfo info;
                info.device = &m_device;
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_compute_shader;
                // info.specialization_map_entries;
                // info.specialization_data;
                m_pipeline = create_compute_pipeline(info);

                m_points_buffer = points;
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_points_buffer = VK_NULL_HANDLE;
                m_pipeline = vulkan::Pipeline();
        }

public:
        Impl(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_device(m_instance.device()),
                  m_shader_memory(m_device),
                  m_compute_shader(m_device, compute_shader, "main"),
                  m_pipeline_layout(
                          vulkan::create_pipeline_layout(m_device, {SET_NUMBER}, {m_shader_memory.descriptor_set_layout()}))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan convex hull compute destructor");
        }
};
}

namespace gpgpu_vulkan
{
std::vector<vulkan::PhysicalDeviceFeatures> ConvexHullCompute::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<ConvexHullCompute> create_convex_hull_compute(const vulkan::VulkanInstance& instance)
{
        return std::make_unique<Impl>(instance);
}
}
