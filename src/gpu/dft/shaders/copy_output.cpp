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

#include "copy_output.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::dft
{
std::vector<VkDescriptorSetLayoutBinding> CopyOutputMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SRC_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DST_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

CopyOutputMemory::CopyOutputMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned CopyOutputMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& CopyOutputMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void CopyOutputMemory::set(const vulkan::BufferWithMemory& input, const vulkan::ImageWithMemory& output) const
{
        {
                ASSERT(input.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = input;
                buffer_info.offset = 0;
                buffer_info.range = input.size();

                m_descriptors.update_descriptor_set(0, SRC_BINDING, buffer_info);
        }
        {
                ASSERT(output.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
                ASSERT(output.format() == VK_FORMAT_R32_SFLOAT);

                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = output.image_view();

                m_descriptors.update_descriptor_set(0, DST_BINDING, image_info);
        }
}

//

CopyOutputConstant::CopyOutputConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, local_size_x);
                entry.size = sizeof(Data::local_size_x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, local_size_y);
                entry.size = sizeof(Data::local_size_y);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, to_mul);
                entry.size = sizeof(Data::to_mul);
                m_entries.push_back(entry);
        }
}

void CopyOutputConstant::set(uint32_t local_size_x, uint32_t local_size_y, float to_mul)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(local_size_x)>);
        m_data.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(local_size_y)>);
        m_data.local_size_y = local_size_y;
        static_assert(std::is_same_v<decltype(m_data.to_mul), decltype(to_mul)>);
        m_data.to_mul = to_mul;
}

const std::vector<VkSpecializationMapEntry>& CopyOutputConstant::entries() const
{
        return m_entries;
}

const void* CopyOutputConstant::data() const
{
        return &m_data;
}

std::size_t CopyOutputConstant::size() const
{
        return sizeof(m_data);
}

//

CopyOutputProgram::CopyOutputProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, CopyOutputMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {CopyOutputMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, code_copy_output_comp(), "main")
{
}

VkDescriptorSetLayout CopyOutputProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout CopyOutputProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline CopyOutputProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void CopyOutputProgram::create_pipeline(uint32_t local_size_x, uint32_t local_size_y, float to_mul)
{
        m_constant.set(local_size_x, local_size_y, to_mul);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void CopyOutputProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
