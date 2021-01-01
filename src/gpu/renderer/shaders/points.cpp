/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "points.h"

#include "descriptors.h"
#include "vertex_points.h"

#include "../code/code.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> PointsProgram::descriptor_set_layout_shared_bindings()
{
        return CommonMemory::descriptor_set_layout_bindings(
                VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                VK_SHADER_STAGE_FRAGMENT_BIT);
}

std::vector<VkDescriptorSetLayoutBinding> PointsProgram::descriptor_set_layout_mesh_bindings()
{
        return MeshMemory::descriptor_set_layout_bindings(VK_SHADER_STAGE_VERTEX_BIT);
}

PointsProgram::PointsProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout_shared(
                  vulkan::create_descriptor_set_layout(device, descriptor_set_layout_shared_bindings())),
          m_descriptor_set_layout_mesh(
                  vulkan::create_descriptor_set_layout(device, descriptor_set_layout_mesh_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {CommonMemory::set_number(), MeshMemory::set_number()},
                  {m_descriptor_set_layout_shared, m_descriptor_set_layout_mesh})),
          m_vertex_shader_0d(m_device, code_points_0d_vert(), "main"),
          m_vertex_shader_1d(m_device, code_points_1d_vert(), "main"),
          m_fragment_shader(m_device, code_points_frag(), "main")
{
}

VkDescriptorSetLayout PointsProgram::descriptor_set_layout_shared() const
{
        return m_descriptor_set_layout_shared;
}

VkDescriptorSetLayout PointsProgram::descriptor_set_layout_mesh() const
{
        return m_descriptor_set_layout_mesh;
}

VkPipelineLayout PointsProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline PointsProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        VkPrimitiveTopology primitive_topology,
        const Region<2, int>& viewport,
        bool transparency) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = false;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport = viewport;
        info.primitive_topology = primitive_topology;

        CommonConstants common_constants;
        common_constants.set(transparency);
        info.depth_write = !transparency;

        std::vector<const vulkan::Shader*> shaders;
        if (primitive_topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
        {
                shaders = {&m_vertex_shader_0d, &m_fragment_shader};
        }
        else if (primitive_topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
        {
                shaders = {&m_vertex_shader_1d, &m_fragment_shader};
        }
        else
        {
                error_fatal("Unsupported primitive topology for renderer points program");
        }
        const std::vector<const vulkan::SpecializationConstant*> constants = {&common_constants, &common_constants};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions = PointsVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                PointsVertex::attribute_descriptions();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
