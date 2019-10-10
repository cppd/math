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

#include "shader_source.h"

#include "gpu/convex_hull/com/com.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

namespace gpu_vulkan
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
          m_shader(instance.device(), convex_hull_prepare_comp(), "main"),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()}, {m_memory.descriptor_set_layout()}))
{
}

void ConvexHullProgramPrepare::create_buffers(const vulkan::ImageWithMemory& objects, unsigned x, unsigned y, unsigned width,
                                              unsigned height, const vulkan::BufferWithMemory& lines_buffer)
{
        ASSERT(width > 0 && height > 0);
        ASSERT(x + width <= objects.width());
        ASSERT(y + height <= objects.height());

        m_height = height;

        m_memory.set_object_image(objects);
        m_memory.set_lines(lines_buffer);

        int buffer_and_group_size = group_size_prepare(objects.width(), m_instance.limits());
        m_constant.set(buffer_and_group_size, buffer_and_group_size, x, y, width, height);

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
          m_shader(instance.device(), convex_hull_merge_comp(), "main"),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()}, {m_memory.descriptor_set_layout()}))
{
}

void ConvexHullProgramMerge::create_buffers(unsigned height, const vulkan::BufferWithMemory& lines_buffer)
{
        m_memory.set_lines(lines_buffer);

        m_constant.set_line_size(height);
        m_constant.set_local_size_x(group_size_merge(height, m_instance.limits()));
        m_constant.set_iteration_count(convex_hull_iteration_count_merge(height));

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
          m_shader(instance.device(), convex_hull_filter_comp(), "main"),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()}, {m_memory.descriptor_set_layout()}))
{
}

void ConvexHullProgramFilter::create_buffers(unsigned height, const vulkan::BufferWithMemory& lines_buffer,
                                             const vulkan::BufferWithMemory& points_buffer,
                                             const vulkan::BufferWithMemory& point_count_buffer)
{
        m_memory.set_lines(lines_buffer);
        m_memory.set_points(points_buffer);
        m_memory.set_point_count(point_count_buffer);

        m_constant.set_line_size(height);

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
