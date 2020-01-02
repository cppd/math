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

#include "show_shader.h"

#include "shader_source.h"

#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> ConvexHullShowMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

ConvexHullShowMemory::ConvexHullShowMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout,
                                           const std::unordered_set<uint32_t>& family_indices)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(vulkan::BufferMemoryType::HostVisible, device, family_indices,
                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Data));
                m_data_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);

                bindings.push_back(DATA_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned ConvexHullShowMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& ConvexHullShowMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void ConvexHullShowMemory::set_matrix(const mat4& matrix) const
{
        decltype(Data().matrix) m = transpose(to_matrix<float>(matrix));
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_data_buffer_index], offsetof(Data, matrix), m);
}

void ConvexHullShowMemory::set_brightness(float brightness) const
{
        decltype(Data().brightness) b = brightness;
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_data_buffer_index], offsetof(Data, brightness), b);
}

void ConvexHullShowMemory::set_points(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, POINTS_BINDING, buffer_info);
}

//

ConvexHullShowProgram::ConvexHullShowProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, ConvexHullShowMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {ConvexHullShowMemory::set_number()}, {m_descriptor_set_layout})),
          m_vertex_shader(m_device, convex_hull_show_vert(), "main"),
          m_fragment_shader(m_device, convex_hull_show_frag(), "main")
{
}

VkDescriptorSetLayout ConvexHullShowProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout ConvexHullShowProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline ConvexHullShowProgram::create_pipeline(VkRenderPass render_pass, VkSampleCountFlagBits sample_count,
                                                        bool sample_shading, unsigned x, unsigned y, unsigned width,
                                                        unsigned height) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport_x = x;
        info.viewport_y = y;
        info.viewport_width = width;
        info.viewport_height = height;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        info.depth_bias = false;
        info.color_blend = false;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader, &m_fragment_shader};
        info.shaders = &shaders;

        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr};
        info.constants = &constants;

        const std::vector<VkVertexInputBindingDescription> binding_descriptions;
        info.binding_descriptions = &binding_descriptions;

        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
