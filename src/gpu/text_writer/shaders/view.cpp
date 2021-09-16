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

#include "view.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::text_writer
{
std::vector<VkDescriptorSetLayoutBinding> Memory::descriptor_set_layout_bindings()
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

Memory::Memory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<uint32_t>& family_indices,
        VkSampler sampler,
        const vulkan::ImageWithMemory* texture)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                uniform_buffers_.emplace_back(
                        vulkan::BufferMemoryType::HOST_VISIBLE, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrices));
                matrices_buffer_index_ = uniform_buffers_.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = uniform_buffers_.back().buffer();
                buffer_info.offset = 0;
                buffer_info.range = uniform_buffers_.back().size();

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
                uniform_buffers_.emplace_back(
                        vulkan::BufferMemoryType::HOST_VISIBLE, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Drawing));
                drawing_buffer_index_ = uniform_buffers_.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = uniform_buffers_.back().buffer();
                buffer_info.offset = 0;
                buffer_info.range = uniform_buffers_.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(DRAWING_BINDING);
        }

        descriptors_.update_descriptor_set(0, bindings, infos);
}

unsigned Memory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& Memory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

template <typename T>
void Memory::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(uniform_buffers_[matrices_buffer_index_], offset, data);
}
template <typename T>
void Memory::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(uniform_buffers_[drawing_buffer_index_], offset, data);
}

void Memory::set_matrix(const Matrix4d& matrix) const
{
        decltype(Matrices().matrix) m = to_matrix<float>(matrix).transpose();
        copy_to_matrices_buffer(offsetof(Matrices, matrix), m);
}

void Memory::set_color(const Vector3f& color) const
{
        decltype(Drawing().color) c = color;
        copy_to_drawing_buffer(offsetof(Drawing, color), c);
}

//

std::vector<VkVertexInputBindingDescription> Vertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;

        {
                VkVertexInputBindingDescription d = {};
                d.binding = 0;
                d.stride = sizeof(Vertex);
                d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> Vertex::attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32_SINT;
                d.offset = offsetof(Vertex, window_coordinates);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 1;
                d.format = VK_FORMAT_R32G32_SFLOAT;
                d.offset = offsetof(Vertex, texture_coordinates);

                descriptions.push_back(d);
        }

        return descriptions;
}

//

Program::Program(const vulkan::Device& device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, Memory::descriptor_set_layout_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(device, {Memory::set_number()}, {descriptor_set_layout_})),
          vertex_shader_(device_, code_view_vert(), "main"),
          fragment_shader_(device_, code_view_frag(), "main")
{
}

VkDescriptorSetLayout Program::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout Program::pipeline_layout() const
{
        return pipeline_layout_;
}

vulkan::Pipeline Program::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        bool sample_shading,
        const Region<2, int>& viewport) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &device_;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        info.color_blend.emplace();
        info.color_blend->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                           | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        info.color_blend->blendEnable = VK_TRUE;
        info.color_blend->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        info.color_blend->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        info.color_blend->colorBlendOp = VK_BLEND_OP_ADD;
        info.color_blend->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        info.color_blend->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        info.color_blend->alphaBlendOp = VK_BLEND_OP_ADD;

        const std::vector<const vulkan::Shader*> shaders = {&vertex_shader_, &fragment_shader_};
        info.shaders = &shaders;

        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr};
        info.constants = &constants;

        const std::vector<VkVertexInputBindingDescription> binding_descriptions = Vertex::binding_descriptions();
        info.binding_descriptions = &binding_descriptions;

        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions = Vertex::attribute_descriptions();
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
