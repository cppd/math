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

#include "shader_normals.h"

#include "shader_source.h"
#include "shader_vertex.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> RendererNormalsMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = MATRICES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DRAWING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

RendererNormalsMemory::RendererNormalsMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const RendererBuffers& buffers)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffers.matrices_buffer();
                buffer_info.offset = 0;
                buffer_info.range = buffers.matrices_size();

                infos.emplace_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffers.drawing_buffer();
                buffer_info.offset = 0;
                buffer_info.range = buffers.drawing_size();

                infos.emplace_back(buffer_info);

                bindings.push_back(DRAWING_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned RendererNormalsMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& RendererNormalsMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

//

RendererNormalsProgram::RendererNormalsProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(vulkan::create_descriptor_set_layout(
                  device,
                  RendererNormalsMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {RendererNormalsMemory::set_number()},
                  {m_descriptor_set_layout})),
          m_vertex_shader(m_device, renderer_normals_vert(), "main"),
          m_geometry_shader(m_device, renderer_normals_geom(), "main"),
          m_fragment_shader(m_device, renderer_normals_frag(), "main")
{
}

VkDescriptorSetLayout RendererNormalsProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout RendererNormalsProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline RendererNormalsProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        bool sample_shading,
        const Region<2, int>& rectangle) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport = rectangle;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        info.depth_bias = false;
        info.color_blend = false;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader, &m_geometry_shader, &m_fragment_shader};
        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr, nullptr};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions =
                RendererTrianglesVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                RendererTrianglesVertex::attribute_descriptions_normals();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
