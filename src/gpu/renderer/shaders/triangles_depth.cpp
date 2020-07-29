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

#include "triangles_depth.h"

#include "common.h"
#include "vertex_triangles.h"

#include "code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> TrianglesDepthProgram::descriptor_set_layout_shared_bindings()
{
        return CommonMemory::descriptor_set_layout_bindings(
                VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_VERTEX_BIT, 0, 0);
}

std::vector<VkDescriptorSetLayoutBinding> TrianglesDepthProgram::descriptor_set_layout_mesh_bindings()
{
        return MeshMemory::descriptor_set_layout_bindings(VK_SHADER_STAGE_VERTEX_BIT);
}

TrianglesDepthProgram::TrianglesDepthProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout_shared(
                  vulkan::create_descriptor_set_layout(device, descriptor_set_layout_shared_bindings())),
          m_descriptor_set_layout_mesh(
                  vulkan::create_descriptor_set_layout(device, descriptor_set_layout_mesh_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {CommonMemory::set_number(), MeshMemory::set_number()},
                  {m_descriptor_set_layout_shared, m_descriptor_set_layout_mesh})),
          m_vertex_shader(m_device, code_triangles_depth_vert(), "main")
{
}

VkDescriptorSetLayout TrianglesDepthProgram::descriptor_set_layout_shared() const
{
        return m_descriptor_set_layout_shared;
}

VkDescriptorSetLayout TrianglesDepthProgram::descriptor_set_layout_mesh() const
{
        return m_descriptor_set_layout_mesh;
}

VkPipelineLayout TrianglesDepthProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline TrianglesDepthProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        const Region<2, int>& viewport) const
{
        ASSERT(sample_count = VK_SAMPLE_COUNT_1_BIT);
        ASSERT(viewport.is_positive());

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = false;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_bias = true;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader};
        info.shaders = &shaders;

        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr};
        info.constants = &constants;

        const std::vector<VkVertexInputBindingDescription> binding_descriptions =
                TrianglesVertex::binding_descriptions();
        info.binding_descriptions = &binding_descriptions;

        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                TrianglesVertex::attribute_descriptions_triangles_depth();
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
