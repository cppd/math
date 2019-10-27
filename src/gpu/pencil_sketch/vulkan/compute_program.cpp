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

#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

namespace gpu_vulkan
{
PencilSketchComputeProgram::PencilSketchComputeProgram(const vulkan::VulkanInstance& instance)
        : m_instance(instance),
          m_descriptor_set_layout(vulkan::create_descriptor_set_layout(
                  instance.device(), PencilSketchComputeMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(instance.device(), {PencilSketchComputeMemory::set_number()},
                                                           {m_descriptor_set_layout})),
          m_shader(instance.device(), pencil_sketch_compute_comp(), "main")
{
}

VkDescriptorSetLayout PencilSketchComputeProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout PencilSketchComputeProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline PencilSketchComputeProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void PencilSketchComputeProgram::create_pipeline(unsigned group_size, unsigned x, unsigned y, unsigned width, unsigned height)
{
        m_constant.set(group_size, x, y, width, height);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_instance.device();
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void PencilSketchComputeProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
