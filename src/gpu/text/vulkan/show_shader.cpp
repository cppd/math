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

#include <src/graphics/vulkan/create.h>
#include <src/graphics/vulkan/pipeline.h>
#include <src/text/vertices.h>

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> TextShowMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = MATRICES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TEXTURE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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

TextShowMemory::TextShowMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::unordered_set<uint32_t>& family_indices,
        VkSampler sampler,
        const vulkan::ImageWithMemory* texture)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(
                        vulkan::BufferMemoryType::HostVisible, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrices));
                m_matrices_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = texture->image_view();
                image_info.sampler = sampler;

                infos.emplace_back(image_info);

                bindings.push_back(TEXTURE_BINDING);
        }
        {
                m_uniform_buffers.emplace_back(
                        vulkan::BufferMemoryType::HostVisible, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Drawing));
                m_drawing_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(DRAWING_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned TextShowMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& TextShowMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

template <typename T>
void TextShowMemory::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_matrices_buffer_index], offset, data);
}
template <typename T>
void TextShowMemory::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_drawing_buffer_index], offset, data);
}

void TextShowMemory::set_matrix(const mat4& matrix) const
{
        decltype(Matrices().matrix) m = to_matrix<float>(matrix).transpose();
        copy_to_matrices_buffer(offsetof(Matrices, matrix), m);
}

void TextShowMemory::set_color(const Color& color) const
{
        decltype(Drawing().color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, color), c);
}

//

static_assert(sizeof(TextVertex) == sizeof(Vector<2, int32_t>) + sizeof(Vector<2, float>));
static_assert(std::is_same_v<decltype(TextVertex::v), Vector<2, int32_t>>);
static_assert(std::is_same_v<decltype(TextVertex::t), Vector<2, float>>);

std::vector<VkVertexInputBindingDescription> TextShowVertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;

        {
                VkVertexInputBindingDescription d = {};
                d.binding = 0;
                d.stride = sizeof(TextVertex);
                d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> TextShowVertex::attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32_SINT;
                d.offset = offsetof(TextVertex, v);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 1;
                d.format = VK_FORMAT_R32G32_SFLOAT;
                d.offset = offsetof(TextVertex, t);

                descriptions.push_back(d);
        }

        return descriptions;
}

//

TextShowProgram::TextShowProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, TextShowMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {TextShowMemory::set_number()}, {m_descriptor_set_layout})),
          m_vertex_shader(m_device, text_vert(), "main"),
          m_fragment_shader(m_device, text_frag(), "main")
{
}

VkDescriptorSetLayout TextShowProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout TextShowProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline TextShowProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        bool sample_shading,
        unsigned x,
        unsigned y,
        unsigned width,
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
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_bias = false;
        info.color_blend = true;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader, &m_fragment_shader};
        info.shaders = &shaders;

        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr};
        info.constants = &constants;

        const std::vector<VkVertexInputBindingDescription> binding_descriptions =
                TextShowVertex::binding_descriptions();
        info.binding_descriptions = &binding_descriptions;

        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                TextShowVertex::attribute_descriptions();
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
