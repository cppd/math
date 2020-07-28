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

#include "triangle_lines.h"

#include "common.h"
#include "vertex_triangles.h"

#include "code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> TriangleLinesMeshMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

vulkan::Descriptors TriangleLinesMeshMemory::create(
        VkDevice device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<CoordinatesInfo>& coordinates)
{
        ASSERT(!coordinates.empty());
        ASSERT(std::all_of(coordinates.cbegin(), coordinates.cend(), [](const CoordinatesInfo& m) {
                return m.buffer != VK_NULL_HANDLE;
        }));

        vulkan::Descriptors descriptors(vulkan::Descriptors(
                device, coordinates.size(), descriptor_set_layout, descriptor_set_layout_bindings()));

        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        for (size_t i = 0; i < coordinates.size(); ++i)
        {
                const CoordinatesInfo& coordinates_info = coordinates[i];

                infos.clear();
                bindings.clear();
                {
                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = coordinates_info.buffer;
                        buffer_info.offset = 0;
                        buffer_info.range = coordinates_info.buffer_size;

                        infos.emplace_back(buffer_info);

                        bindings.push_back(BUFFER_BINDING);
                }
                descriptors.update_descriptor_set(i, bindings, infos);
        }

        return descriptors;
}

unsigned TriangleLinesMeshMemory::set_number()
{
        return SET_NUMBER;
}

//

std::vector<VkDescriptorSetLayoutBinding> TriangleLinesProgram::descriptor_set_layout_shared_bindings()
{
        return CommonMemory::descriptor_set_layout_bindings(
                VK_SHADER_STAGE_GEOMETRY_BIT, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 0);
}

TriangleLinesProgram::TriangleLinesProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout_shared(
                  vulkan::create_descriptor_set_layout(device, descriptor_set_layout_shared_bindings())),
          m_descriptor_set_layout_mesh(vulkan::create_descriptor_set_layout(
                  device,
                  TriangleLinesMeshMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {CommonMemory::set_number(), TriangleLinesMeshMemory::set_number()},
                  {m_descriptor_set_layout_shared, m_descriptor_set_layout_mesh})),
          m_vertex_shader(m_device, code_triangle_lines_vert(), "main"),
          m_geometry_shader(m_device, code_triangle_lines_geom(), "main"),
          m_fragment_shader(m_device, code_triangle_lines_frag(), "main")
{
}

VkDescriptorSetLayout TriangleLinesProgram::descriptor_set_layout_shared() const
{
        return m_descriptor_set_layout_shared;
}

VkDescriptorSetLayout TriangleLinesProgram::descriptor_set_layout_mesh() const
{
        return m_descriptor_set_layout_mesh;
}

VkPipelineLayout TriangleLinesProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline TriangleLinesProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        bool sample_shading,
        const Region<2, int>& viewport) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader, &m_geometry_shader, &m_fragment_shader};
        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr, nullptr};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions =
                TrianglesVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                TrianglesVertex::attribute_descriptions_triangle_lines();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
