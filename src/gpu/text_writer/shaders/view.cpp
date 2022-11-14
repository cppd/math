/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <src/vulkan/pipeline_graphics.h>

namespace ns::gpu::text_writer
{
Buffer::Buffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : buffer_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Data))
{
}

const vulkan::Buffer& Buffer::buffer() const
{
        return buffer_.buffer();
}

void Buffer::set_matrix(const Matrix4d& matrix) const
{
        decltype(Data().matrix) m = vulkan::to_std140<float>(matrix);
        vulkan::map_and_write_to_buffer(buffer_, offsetof(Data, matrix), m);
}

void Buffer::set_color(const Vector3f& color) const
{
        decltype(Data().color) c = color;
        vulkan::map_and_write_to_buffer(buffer_, offsetof(Data, color), c);
}

//

std::vector<VkDescriptorSetLayoutBinding> Memory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = DATA_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = TEXTURE_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

Memory::Memory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& data_buffer)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = data_buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = data_buffer.size();

        descriptors_.update_descriptor_set(0, DATA_BINDING, buffer_info);
}

unsigned Memory::set_number()
{
        return SET_NUMBER;
}

void Memory::set_image(const VkSampler sampler, const VkImageView image) const
{
        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image;
        image_info.sampler = sampler;

        descriptors_.update_descriptor_set(0, TEXTURE_BINDING, image_info);
}

const VkDescriptorSet& Memory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
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

Program::Program(const vulkan::Device* const device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device_->handle(), Memory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device_->handle(), {Memory::set_number()}, {descriptor_set_layout_})),
          vertex_shader_(device_->handle(), code_view_vert(), VK_SHADER_STAGE_VERTEX_BIT),
          fragment_shader_(device_->handle(), code_view_frag(), VK_SHADER_STAGE_FRAGMENT_BIT)
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

vulkan::handle::Pipeline Program::create_pipeline(
        const vulkan::RenderPass& render_pass,
        const VkSampleCountFlagBits sample_count,
        const bool sample_shading,
        const Region<2, int>& viewport) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = &render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.shaders = {&vertex_shader_, &fragment_shader_};
        info.binding_descriptions = Vertex::binding_descriptions();
        info.attribute_descriptions = Vertex::attribute_descriptions();

        ASSERT(render_pass.color_attachment_count() == 1);
        VkPipelineColorBlendAttachmentState& state = info.color_blend.emplace_back();
        state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                               | VK_COLOR_COMPONENT_A_BIT;
        state.blendEnable = VK_TRUE;
        state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        state.colorBlendOp = VK_BLEND_OP_ADD;
        state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        state.alphaBlendOp = VK_BLEND_OP_ADD;

        return vulkan::create_graphics_pipeline(info);
}
}
