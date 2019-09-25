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

#include "com/groups.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

constexpr unsigned GROUP_SIZE = 16;

namespace gpu_vulkan
{
PencilSketchComputeProgram::PencilSketchComputeProgram(const vulkan::VulkanInstance& instance)
        : m_instance(instance),
          m_memory(instance.device()),
          m_shader(instance.device(), pencil_sketch_compute_comp(), "main"),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()}, {m_memory.descriptor_set_layout()}))
{
}

void PencilSketchComputeProgram::create_buffers(VkSampler sampler, const vulkan::ImageWithMemory& input_image,
                                                const vulkan::ImageWithMemory& object_image,
                                                const vulkan::ImageWithMemory& output_image)
{
        ASSERT(sampler != VK_NULL_HANDLE);
        ASSERT(input_image.width() == object_image.width() && input_image.height() == object_image.height());
        ASSERT(input_image.width() == output_image.width() && input_image.height() == output_image.height());

        m_groups_x = group_count(input_image.width(), GROUP_SIZE);
        m_groups_y = group_count(input_image.height(), GROUP_SIZE);

        m_memory.set_input(sampler, input_image);
        m_memory.set_object_image(object_image);
        m_memory.set_output_image(output_image);

        m_constant.set_group_size(GROUP_SIZE);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_instance.device();
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void PencilSketchComputeProgram::delete_buffers()
{
        m_pipeline = vulkan::Pipeline();
        m_groups_x = 0;
        m_groups_y = 0;
}

void PencilSketchComputeProgram::commands(VkCommandBuffer command_buffer) const
{
        ASSERT(m_groups_x > 0 && m_groups_y > 0);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, m_memory.set_number(),
                                1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);
        vkCmdDispatch(command_buffer, m_groups_x, m_groups_y, 1);
}
}
