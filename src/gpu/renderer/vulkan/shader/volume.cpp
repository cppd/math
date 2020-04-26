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

#include "volume.h"

#include "../../shaders/code.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> VolumeMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = VOLUME_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DRAWING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

VolumeMemory::VolumeMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& volume,
        const vulkan::Buffer& drawing)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = volume;
                buffer_info.offset = 0;
                buffer_info.range = volume.size();

                infos.emplace_back(buffer_info);

                bindings.push_back(VOLUME_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = drawing;
                buffer_info.offset = 0;
                buffer_info.range = drawing.size();

                infos.emplace_back(buffer_info);

                bindings.push_back(DRAWING_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned VolumeMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& VolumeMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

//

VolumeProgram::VolumeProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, VolumeMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {VolumeMemory::set_number()}, {m_descriptor_set_layout})),
          m_vertex_shader(m_device, code_volume_vert(), "main"),
          m_fragment_shader(m_device, code_volume_frag(), "main")
{
}

VkDescriptorSetLayout VolumeProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout VolumeProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline VolumeProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        bool sample_shading,
        const Region<2, int>& viewport) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_shading ? sample_count : VK_SAMPLE_COUNT_1_BIT;
        info.sample_shading = sample_shading;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_bias = false;
        info.color_blend = false;
        info.depth_test = false;
        info.depth_write = false;

        std::vector<const vulkan::Shader*> shaders{&m_vertex_shader, &m_fragment_shader};
        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions;
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
