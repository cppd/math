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

/*
По книге

Satyan L. Devadoss, Joseph O’Rourke.
Discrete and computational geometry.
Princeton University Press, 2011.

Chapter 2: CONVEX HULLS, 2.6 Divide-and-Conquer.
*/

#include "compute.h"

#include "size.h"

#include "shaders/filter.h"
#include "shaders/merge.h"
#include "shaders/prepare.h"

#include <src/com/error.h>

#include <optional>
#include <thread>

namespace ns::gpu::convex_hull
{
namespace
{
// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

int group_size_merge(int height, const VkPhysicalDeviceLimits& limits)
{
        return convex_hull::group_size_merge(
                height, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                limits.maxComputeSharedMemorySize);
}

int group_size_prepare(int width, const VkPhysicalDeviceLimits& limits)
{
        return convex_hull::group_size_prepare(
                width, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                limits.maxComputeSharedMemorySize);
}

void buffer_barrier(
        VkCommandBuffer command_buffer,
        VkBuffer buffer,
        VkAccessFlags dst_access_mask,
        VkPipelineStageFlags dst_stage_mask)
{
        ASSERT(buffer != VK_NULL_HANDLE);

        VkBufferMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = dst_access_mask;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer;
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dst_stage_mask, VK_DEPENDENCY_BY_REGION_BIT, 0,
                nullptr, 1, &barrier, 0, nullptr);
}

class Impl final : public Compute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;

        std::optional<vulkan::BufferWithMemory> m_lines_buffer;
        VkBuffer m_points_buffer = VK_NULL_HANDLE;
        VkBuffer m_point_count_buffer = VK_NULL_HANDLE;

        unsigned m_prepare_group_count = 0;
        PrepareProgram m_prepare_program;
        PrepareMemory m_prepare_memory;

        MergeProgram m_merge_program;
        MergeMemory m_merge_memory;

        FilterProgram m_filter_program;
        FilterMemory m_filter_memory;

        void compute_commands(VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_prepare_program.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_prepare_program.pipeline_layout(),
                        PrepareMemory::set_number(), 1, &m_prepare_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_prepare_group_count, 1, 1);

                //

                buffer_barrier(
                        command_buffer, *m_lines_buffer, VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_merge_program.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_merge_program.pipeline_layout(),
                        MergeMemory::set_number(), 1, &m_merge_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, 2, 1, 1);

                //

                buffer_barrier(
                        command_buffer, *m_lines_buffer, VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_filter_program.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_filter_program.pipeline_layout(),
                        FilterMemory::set_number(), 1, &m_filter_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, 1, 1, 1);

                //

                buffer_barrier(
                        command_buffer, m_points_buffer, VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
                buffer_barrier(
                        command_buffer, m_point_count_buffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
        }

        void create_buffers(
                const vulkan::ImageWithMemory& objects,
                const Region<2, int>& rectangle,
                const vulkan::BufferWithMemory& points_buffer,
                const vulkan::BufferWithMemory& point_count_buffer,
                uint32_t family_index) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(rectangle.is_positive());
                ASSERT(rectangle.x1() <= static_cast<int>(objects.width()));
                ASSERT(rectangle.y1() <= static_cast<int>(objects.height()));

                ASSERT(points_buffer.size() == (2 * rectangle.height() + 1) * (2 * sizeof(int32_t)));
                ASSERT(point_count_buffer.size() >= sizeof(int32_t));

                const int width = rectangle.width();
                const int height = rectangle.height();

                m_lines_buffer.emplace(
                        vulkan::BufferMemoryType::DeviceLocal, m_instance.device(), std::unordered_set({family_index}),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 2 * height * sizeof(int32_t));
                m_points_buffer = points_buffer;
                m_point_count_buffer = point_count_buffer;

                m_prepare_memory.set_object_image(objects);
                m_prepare_memory.set_lines(*m_lines_buffer);
                m_prepare_group_count = height;
                m_prepare_program.create_pipeline(
                        group_size_prepare(width, m_instance.device().properties().properties_10.limits), rectangle);

                m_merge_memory.set_lines(*m_lines_buffer);
                m_merge_program.create_pipeline(
                        height, group_size_merge(height, m_instance.device().properties().properties_10.limits),
                        iteration_count_merge(height));

                m_filter_memory.set_lines(*m_lines_buffer);
                m_filter_memory.set_points(points_buffer);
                m_filter_memory.set_point_count(point_count_buffer);
                m_filter_program.create_pipeline(height);
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_filter_program.delete_pipeline();
                m_merge_program.delete_pipeline();
                m_prepare_program.delete_pipeline();
                m_prepare_group_count = 0;

                m_points_buffer = VK_NULL_HANDLE;
                m_point_count_buffer = VK_NULL_HANDLE;
                m_lines_buffer.reset();
        }

public:
        explicit Impl(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_prepare_program(instance.device()),
                  m_prepare_memory(instance.device(), m_prepare_program.descriptor_set_layout()),
                  m_merge_program(instance.device()),
                  m_merge_memory(instance.device(), m_merge_program.descriptor_set_layout()),
                  m_filter_program(instance.device()),
                  m_filter_memory(instance.device(), m_filter_program.descriptor_set_layout())
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

std::vector<vulkan::PhysicalDeviceFeatures> Compute::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<Compute> create_compute(const vulkan::VulkanInstance& instance)
{
        return std::make_unique<Impl>(instance);
}
}
