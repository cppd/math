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

#include "compute_program.h"

#include "gpgpu/convex_hull/com/com.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

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

namespace gpgpu_vulkan
{
namespace
{
int group_size_prepare(int width, const VkPhysicalDeviceLimits& limits)
{
        return convex_hull_group_size_prepare(width, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                                              limits.maxComputeSharedMemorySize);
}

int group_size_merge(int height, const VkPhysicalDeviceLimits& limits)
{
        return convex_hull_group_size_merge(height, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                                            limits.maxComputeSharedMemorySize);
}
}

ConvexHullProgramPrepare::ConvexHullProgramPrepare(const vulkan::VulkanInstance& instance)
        : m_instance(instance),
          m_memory(instance.device()),
          m_shader(instance.device(), prepare_shader, "main"),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()}, {m_memory.descriptor_set_layout()}))
{
}

void ConvexHullProgramPrepare::create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer)
{
        m_height = objects.height();

        m_memory.set_object_image(objects);
        m_memory.set_lines(lines_buffer);

        m_constant.set_line_size(objects.height());
        m_constant.set_buffer_and_group_size(group_size_prepare(objects.width(), m_instance.limits()));

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_instance.device();
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void ConvexHullProgramPrepare::delete_buffers()
{
        m_pipeline = vulkan::Pipeline();
        m_height = 0;
}

void ConvexHullProgramPrepare::commands(VkCommandBuffer command_buffer) const
{
        ASSERT(m_height > 0);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, m_memory.set_number(),
                                1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);
        vkCmdDispatch(command_buffer, m_height, 1, 1);
}

//

ConvexHullProgramMerge::ConvexHullProgramMerge(const vulkan::VulkanInstance& instance)
        : m_instance(instance),
          m_memory(instance.device()),
          m_shader(instance.device(), merge_shader, "main"),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()}, {m_memory.descriptor_set_layout()}))
{
}

void ConvexHullProgramMerge::create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer)
{
        m_memory.set_lines(lines_buffer);

        m_constant.set_line_size(objects.height());
        m_constant.set_local_size_x(group_size_merge(objects.height(), m_instance.limits()));
        m_constant.set_iteration_count(convex_hull_iteration_count_merge(objects.height()));

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_instance.device();
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void ConvexHullProgramMerge::delete_buffers()
{
        m_pipeline = vulkan::Pipeline();
}

void ConvexHullProgramMerge::commands(VkCommandBuffer command_buffer) const
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, m_memory.set_number(),
                                1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);
        vkCmdDispatch(command_buffer, 2, 1, 1);
}

//

ConvexHullProgramFilter::ConvexHullProgramFilter(const vulkan::VulkanInstance& instance)
        : m_instance(instance),
          m_memory(instance.device()),
          m_shader(instance.device(), filter_shader, "main"),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()}, {m_memory.descriptor_set_layout()}))
{
}

void ConvexHullProgramFilter::create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer,
                                             const vulkan::BufferWithMemory& points_buffer,
                                             const vulkan::BufferWithMemory& point_count_buffer)
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

void ConvexHullProgramFilter::delete_buffers()
{
        m_pipeline = vulkan::Pipeline();
}

void ConvexHullProgramFilter::commands(VkCommandBuffer command_buffer) const
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, m_memory.set_number(),
                                1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);
        vkCmdDispatch(command_buffer, 1, 1, 1);
}
}
