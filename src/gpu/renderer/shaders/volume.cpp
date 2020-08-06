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

#include "code/code.h"

#include <src/com/error.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu::renderer
{
namespace
{
class Constants final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t drawing_type;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override
        {
                return m_entries;
        }

        const void* data() const override
        {
                return &m_data;
        }

        size_t size() const override
        {
                return sizeof(m_data);
        }

public:
        Constants()
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, drawing_type);
                entry.size = sizeof(Data::drawing_type);
                m_entries.push_back(entry);
        }

        void set_drawing_type(uint32_t drawing_type)
        {
                m_data.drawing_type = drawing_type;
        }
};
}

std::vector<VkDescriptorSetLayoutBinding> VolumeSharedMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DRAWING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DEPTH_IMAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TRANSPARENCY_HEADS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TRANSPARENCY_NODES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

VolumeSharedMemory::VolumeSharedMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const vulkan::Buffer& drawing)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings)
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

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

unsigned VolumeSharedMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& VolumeSharedMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void VolumeSharedMemory::set_depth_image(VkImageView image_view, VkSampler sampler) const
{
        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image_view;
        image_info.sampler = sampler;

        m_descriptors.update_descriptor_set(0, DEPTH_IMAGE_BINDING, image_info);
}

void VolumeSharedMemory::set_transparency(const vulkan::ImageWithMemory& heads, const vulkan::Buffer& nodes) const
{
        ASSERT(heads.format() == VK_FORMAT_R32_UINT);
        ASSERT(heads.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(nodes.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = heads.image_view();

                infos.emplace_back(image_info);
                bindings.push_back(TRANSPARENCY_HEADS_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = nodes;
                buffer_info.offset = 0;
                buffer_info.range = nodes.size();

                infos.emplace_back(buffer_info);

                bindings.push_back(TRANSPARENCY_NODES_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

//

std::vector<VkDescriptorSetLayoutBinding> VolumeImageMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_COORDINATES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_VOLUME_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = IMAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TRANSFER_FUNCTION_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

vulkan::Descriptors VolumeImageMemory::create(
        VkDevice device,
        VkSampler image_sampler,
        VkSampler transfer_function_sampler,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const CreateInfo& create_info)
{
        vulkan::Descriptors descriptors(
                vulkan::Descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings));

        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = create_info.buffer_coordinates;
                buffer_info.offset = 0;
                buffer_info.range = create_info.buffer_coordinates_size;

                infos.emplace_back(buffer_info);

                bindings.push_back(BUFFER_COORDINATES_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = create_info.buffer_volume;
                buffer_info.offset = 0;
                buffer_info.range = create_info.buffer_volume_size;

                infos.emplace_back(buffer_info);

                bindings.push_back(BUFFER_VOLUME_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = create_info.image;
                image_info.sampler = image_sampler;

                infos.emplace_back(image_info);

                bindings.push_back(IMAGE_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = create_info.transfer_function;
                image_info.sampler = transfer_function_sampler;

                infos.emplace_back(image_info);

                bindings.push_back(TRANSFER_FUNCTION_BINDING);
        }

        descriptors.update_descriptor_set(0, bindings, infos);

        return descriptors;
}

unsigned VolumeImageMemory::set_number()
{
        return SET_NUMBER;
}

//

std::vector<VkDescriptorSetLayoutBinding> VolumeProgram::descriptor_set_layout_shared_bindings()
{
        return VolumeSharedMemory::descriptor_set_layout_bindings();
}

std::vector<VkDescriptorSetLayoutBinding> VolumeProgram::descriptor_set_layout_image_bindings()
{
        return VolumeImageMemory::descriptor_set_layout_bindings();
}

VolumeProgram::VolumeProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout_shared(
                  vulkan::create_descriptor_set_layout(device, descriptor_set_layout_shared_bindings())),
          m_descriptor_set_layout_image(
                  vulkan::create_descriptor_set_layout(device, descriptor_set_layout_image_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {VolumeSharedMemory::set_number(), VolumeImageMemory::set_number()},
                  {m_descriptor_set_layout_shared, m_descriptor_set_layout_image})),
          m_vertex_shader(m_device, code_volume_vert(), "main"),
          m_fragment_shader(m_device, code_volume_frag(), "main")
{
}

VkDescriptorSetLayout VolumeProgram::descriptor_set_layout_shared() const
{
        return m_descriptor_set_layout_shared;
}

VkDescriptorSetLayout VolumeProgram::descriptor_set_layout_image() const
{
        return m_descriptor_set_layout_image;
}

VkPipelineLayout VolumeProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline VolumeProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        bool sample_shading,
        const Region<2, int>& viewport,
        int drawing_type) const
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
        info.depth_test = false;
        info.depth_write = false;

        info.color_blend.emplace();
        info.color_blend->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                           | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        info.color_blend->blendEnable = VK_TRUE;
        info.color_blend->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        info.color_blend->dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        info.color_blend->colorBlendOp = VK_BLEND_OP_ADD;
        info.color_blend->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        info.color_blend->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        info.color_blend->alphaBlendOp = VK_BLEND_OP_ADD;

        Constants constants;
        constants.set_drawing_type(drawing_type);

        std::vector<const vulkan::Shader*> shaders{&m_vertex_shader, &m_fragment_shader};
        const std::vector<const vulkan::SpecializationConstant*> specialization_constants = {nullptr, &constants};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions;
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        info.shaders = &shaders;
        info.constants = &specialization_constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
