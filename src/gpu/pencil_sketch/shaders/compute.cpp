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

#include "compute.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::pencil_sketch
{
std::vector<VkDescriptorSetLayoutBinding> ComputeMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = INPUT_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OUTPUT_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OBJECTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

unsigned ComputeMemory::set_number()
{
        return SET_NUMBER;
}

ComputeMemory::ComputeMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

const VkDescriptorSet& ComputeMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void ComputeMemory::set_input(VkSampler sampler, const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image.image_view();
        image_info.sampler = sampler;

        m_descriptors.update_descriptor_set(0, INPUT_BINDING, image_info);
}

void ComputeMemory::set_output_image(const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.image_view();

        m_descriptors.update_descriptor_set(0, OUTPUT_BINDING, image_info);
}

void ComputeMemory::set_object_image(const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.format() == VK_FORMAT_R32_UINT);
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.image_view();

        m_descriptors.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

//

ComputeConstant::ComputeConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, local_size);
                entry.size = sizeof(Data::local_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, x);
                entry.size = sizeof(Data::x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, y);
                entry.size = sizeof(Data::y);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, width);
                entry.size = sizeof(Data::width);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, height);
                entry.size = sizeof(Data::height);
                m_entries.push_back(entry);
        }
}

void ComputeConstant::set(int32_t local_size, const Region<2, int>& rectangle)
{
        static_assert(std::is_same_v<decltype(m_data.local_size), decltype(local_size)>);
        m_data.local_size = local_size;

        ASSERT(rectangle.is_positive());
        m_data.x = rectangle.x0();
        m_data.y = rectangle.y0();
        m_data.width = rectangle.width();
        m_data.height = rectangle.height();
}

const std::vector<VkSpecializationMapEntry>& ComputeConstant::entries() const
{
        return m_entries;
}

const void* ComputeConstant::data() const
{
        return &m_data;
}

std::size_t ComputeConstant::size() const
{
        return sizeof(m_data);
}

//

ComputeProgram::ComputeProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, ComputeMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {ComputeMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, code_compute_comp(), "main")
{
}

VkDescriptorSetLayout ComputeProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout ComputeProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline ComputeProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void ComputeProgram::create_pipeline(unsigned group_size, const Region<2, int>& rectangle)
{
        m_constant.set(group_size, rectangle);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void ComputeProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
