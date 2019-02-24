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

#include "com/error.h"
#include "gpgpu/convex_hull/compute/objects/com.h"
#include "gpgpu/convex_hull/compute/objects/vulkan_shader.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/shader.h"

#include <optional>
#include <thread>

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

// clang-format off
constexpr uint32_t prepare_shader[]
{
#include "ch_prepare.comp.spr"
};
constexpr uint32_t merge_shader[]
{
#include "ch_merge.comp.spr"
};
constexpr uint32_t filter_shader[]
{
#include "ch_filter.comp.spr"
};
// clang-format on

namespace
{
namespace impl
{
using namespace gpgpu_convex_hull_compute_implementation;
using namespace gpgpu_convex_hull_compute_vulkan_implementation;
}

int group_size_prepare(int width, const VkPhysicalDeviceLimits& limits)
{
        return impl::group_size_prepare(width, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                                        limits.maxComputeSharedMemorySize);
}

int group_size_merge(int height, const VkPhysicalDeviceLimits& limits)
{
        return impl::group_size_merge(height, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                                      limits.maxComputeSharedMemorySize);
}

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

class ProgramPrepare
{
        const vulkan::VulkanInstance& m_instance;

        impl::PrepareMemory m_memory;
        impl::PrepareConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

        unsigned m_height = 0;

public:
        ProgramPrepare(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_memory(instance.device()),
                  m_shader(instance.device(), prepare_shader, "main"),
                  m_pipeline_layout(vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()},
                                                                   {m_memory.descriptor_set_layout()}))
        {
        }

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithHostVisibleMemory& lines_buffer)
        {
                m_height = objects.height();

                m_memory.set_object_image(objects);
                m_memory.set_lines(lines_buffer);

                m_constant.set_line_size(objects.height());
                m_constant.set_buffer_and_group_size(
                        group_size_prepare(objects.width(), m_instance.physical_device().properties().limits));

                vulkan::ComputePipelineCreateInfo info;
                info.device = &m_instance.device();
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_shader;
                info.constants = &m_constant;
                m_pipeline = create_compute_pipeline(info);
        }

        void delete_buffers()
        {
                m_pipeline = vulkan::Pipeline();
                m_height = 0;
        }

        void commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(m_height > 0);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, m_memory.set_number(),
                                        1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_height, 1, 1);
        }
};

class ProgramMerge
{
        const vulkan::VulkanInstance& m_instance;

        impl::MergeMemory m_memory;
        impl::MergeConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

public:
        ProgramMerge(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_memory(instance.device()),
                  m_shader(instance.device(), merge_shader, "main"),
                  m_pipeline_layout(vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()},
                                                                   {m_memory.descriptor_set_layout()}))
        {
        }

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithHostVisibleMemory& lines_buffer)
        {
                m_memory.set_lines(lines_buffer);

                m_constant.set_line_size(objects.height());
                m_constant.set_local_size_x(group_size_merge(objects.height(), m_instance.physical_device().properties().limits));
                m_constant.set_iteration_count(impl::iteration_count_merge(objects.height()));

                vulkan::ComputePipelineCreateInfo info;
                info.device = &m_instance.device();
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_shader;
                info.constants = &m_constant;
                m_pipeline = create_compute_pipeline(info);
        }

        void delete_buffers()
        {
                m_pipeline = vulkan::Pipeline();
        }

        void commands(VkCommandBuffer command_buffer) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, m_memory.set_number(),
                                        1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, 2, 1, 1);
        }
};

class ProgramFilter
{
        const vulkan::VulkanInstance& m_instance;

        impl::FilterMemory m_memory;
        impl::FilterConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

public:
        ProgramFilter(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_memory(instance.device()),
                  m_shader(instance.device(), filter_shader, "main"),
                  m_pipeline_layout(vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()},
                                                                   {m_memory.descriptor_set_layout()}))
        {
        }

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithHostVisibleMemory& lines_buffer,
                            const vulkan::BufferWithHostVisibleMemory& points_buffer,
                            const vulkan::BufferWithHostVisibleMemory& point_count_buffer)
        {
                m_memory.set_lines(lines_buffer);
                m_memory.set_points(points_buffer);
                m_memory.set_point_count(point_count_buffer);

                m_constant.set_line_size(objects.height());

                vulkan::ComputePipelineCreateInfo info;
                info.device = &m_instance.device();
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_shader;
                info.constants = &m_constant;
                m_pipeline = create_compute_pipeline(info);
        }

        void delete_buffers()
        {
                m_pipeline = vulkan::Pipeline();
        }

        void commands(VkCommandBuffer command_buffer) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, m_memory.set_number(),
                                        1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, 1, 1, 1);
        }
};

class Impl final : public gpgpu_vulkan::ConvexHullCompute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;

        std::optional<vulkan::BufferWithHostVisibleMemory> m_lines_buffer;
        VkBuffer m_points_buffer = VK_NULL_HANDLE;
        VkBuffer m_point_count_buffer = VK_NULL_HANDLE;

        ProgramPrepare m_program_prepare;
        ProgramMerge m_program_merge;
        ProgramFilter m_program_filter;

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

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithHostVisibleMemory& points_buffer,
                            const vulkan::BufferWithHostVisibleMemory& point_count_buffer) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(points_buffer.size() == (2 * objects.height() + 1) * (2 * sizeof(int32_t)));
                ASSERT(point_count_buffer.size() >= sizeof(int32_t));

                m_lines_buffer.emplace(m_instance.device(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
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
