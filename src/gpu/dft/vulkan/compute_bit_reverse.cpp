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

#include "compute_bit_reverse.h"

#include "shader_source.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> DftBitReverseMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftBitReverseMemory::DftBitReverseMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftBitReverseMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& DftBitReverseMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftBitReverseMemory::set_buffer(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

DftBitReverseConstant::DftBitReverseConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, group_size);
                entry.size = sizeof(Data::group_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, data_size);
                entry.size = sizeof(Data::data_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, n_mask);
                entry.size = sizeof(Data::n_mask);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, n_bits);
                entry.size = sizeof(Data::n_bits);
                m_entries.push_back(entry);
        }
}

void DftBitReverseConstant::set(uint32_t group_size, uint32_t data_size, uint32_t n_mask, uint32_t n_bits)
{
        static_assert(std::is_same_v<decltype(m_data.group_size), decltype(group_size)>);
        m_data.group_size = group_size;
        static_assert(std::is_same_v<decltype(m_data.data_size), decltype(data_size)>);
        m_data.data_size = data_size;
        static_assert(std::is_same_v<decltype(m_data.n_mask), decltype(n_mask)>);
        m_data.n_mask = n_mask;
        static_assert(std::is_same_v<decltype(m_data.n_bits), decltype(n_bits)>);
        m_data.n_bits = n_bits;
}

const std::vector<VkSpecializationMapEntry>& DftBitReverseConstant::entries() const
{
        return m_entries;
}

const void* DftBitReverseConstant::data() const
{
        return &m_data;
}

size_t DftBitReverseConstant::size() const
{
        return sizeof(m_data);
}

//

DftBitReverseProgram::DftBitReverseProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, DftBitReverseMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {DftBitReverseMemory::set_number()},
                  {m_descriptor_set_layout})),
          m_shader(device, dft_bit_reverse_comp(), "main")
{
}

VkDescriptorSetLayout DftBitReverseProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout DftBitReverseProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline DftBitReverseProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void DftBitReverseProgram::create_pipeline(uint32_t group_size, uint32_t data_size, uint32_t n_mask, uint32_t n_bits)
{
        m_constant.set(group_size, data_size, n_mask, n_bits);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void DftBitReverseProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
