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

#include "compute.h"

#include "compute_program.h"

#include "com/error.h"

#include <optional>
#include <thread>

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

namespace gpu_vulkan
{
namespace
{
void buffer_barrier(VkCommandBuffer command_buffer, VkBuffer buffer, VkAccessFlags dst_access_mask,
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

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dst_stage_mask, VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, 1, &barrier, 0, nullptr);
}

class Impl final : public ConvexHullCompute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;

        std::optional<vulkan::BufferWithMemory> m_lines_buffer;
        VkBuffer m_points_buffer = VK_NULL_HANDLE;
        VkBuffer m_point_count_buffer = VK_NULL_HANDLE;

        ConvexHullProgramPrepare m_program_prepare;
        ConvexHullProgramMerge m_program_merge;
        ConvexHullProgramFilter m_program_filter;

        void compute_commands(VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_program_prepare.commands(command_buffer);

                buffer_barrier(command_buffer, *m_lines_buffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                m_program_merge.commands(command_buffer);

                buffer_barrier(command_buffer, *m_lines_buffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                m_program_filter.commands(command_buffer);

                buffer_barrier(command_buffer, m_points_buffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
                buffer_barrier(command_buffer, m_point_count_buffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                               VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
        }

        void create_buffers(const vulkan::ImageWithMemory& objects, const vulkan::BufferWithMemory& points_buffer,
                            const vulkan::BufferWithMemory& point_count_buffer, uint32_t family_index) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(points_buffer.size() == (2 * objects.height() + 1) * (2 * sizeof(int32_t)));
                ASSERT(point_count_buffer.size() >= sizeof(int32_t));

                m_lines_buffer.emplace(vulkan::BufferMemoryType::DeviceLocal, m_instance.device(),
                                       std::unordered_set({family_index}), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       2 * objects.height() * sizeof(int32_t));
                m_points_buffer = points_buffer;
                m_point_count_buffer = point_count_buffer;

                m_program_prepare.create_buffers(objects, *m_lines_buffer);
                m_program_merge.create_buffers(objects, *m_lines_buffer);
                m_program_filter.create_buffers(objects, *m_lines_buffer, points_buffer, point_count_buffer);
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_program_filter.delete_buffers();
                m_program_merge.delete_buffers();
                m_program_prepare.delete_buffers();

                m_points_buffer = VK_NULL_HANDLE;
                m_point_count_buffer = VK_NULL_HANDLE;
                m_lines_buffer.reset();
        }

public:
        Impl(const vulkan::VulkanInstance& instance)
                : m_instance(instance), m_program_prepare(instance), m_program_merge(instance), m_program_filter(instance)
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

std::vector<vulkan::PhysicalDeviceFeatures> ConvexHullCompute::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<ConvexHullCompute> create_convex_hull_compute(const vulkan::VulkanInstance& instance)
{
        return std::make_unique<Impl>(instance);
}
}
