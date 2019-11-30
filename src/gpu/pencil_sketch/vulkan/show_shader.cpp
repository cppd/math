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

#include "show_shader.h"

#include "shader_source.h"

#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> PencilSketchShowMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = IMAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

PencilSketchShowMemory::PencilSketchShowMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned PencilSketchShowMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& PencilSketchShowMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void PencilSketchShowMemory::set_image(VkSampler sampler, const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.usage() & VK_IMAGE_USAGE_SAMPLED_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image.image_view();
        image_info.sampler = sampler;

        m_descriptors.update_descriptor_set(0, IMAGE_BINDING, image_info);
}

//

std::vector<VkVertexInputBindingDescription> PencilSketchShowVertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;

        {
                VkVertexInputBindingDescription d = {};
                d.binding = 0;
                d.stride = sizeof(PencilSketchShowVertex);
                d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> PencilSketchShowVertex::attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                d.offset = offsetof(PencilSketchShowVertex, position);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 1;
                d.format = VK_FORMAT_R32G32_SFLOAT;
                d.offset = offsetof(PencilSketchShowVertex, texture_coordinates);

                descriptions.push_back(d);
        }

        return descriptions;
}

//

PencilSketchShowProgram::PencilSketchShowProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, PencilSketchShowMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {PencilSketchShowMemory::set_number()}, {m_descriptor_set_layout})),
          m_vertex_shader(m_device, pencil_sketch_show_vert(), "main"),
          m_fragment_shader(m_device, pencil_sketch_show_frag(), "main")
{
}

VkDescriptorSetLayout PencilSketchShowProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout PencilSketchShowProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline PencilSketchShowProgram::create_pipeline(VkRenderPass render_pass, VkSampleCountFlagBits sample_count,
                                                          unsigned x, unsigned y, unsigned width, unsigned height) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = false;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport_x = x;
        info.viewport_y = y;
        info.viewport_width = width;
        info.viewport_height = height;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        info.depth_bias = false;
        info.color_blend = false;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader, &m_fragment_shader};
        info.shaders = &shaders;

        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr};
        info.constants = &constants;

        const std::vector<VkVertexInputBindingDescription> binding_descriptions = PencilSketchShowVertex::binding_descriptions();
        info.binding_descriptions = &binding_descriptions;

        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                PencilSketchShowVertex::attribute_descriptions();
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
