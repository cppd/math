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

#include "compute_mul_d.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> DftMulDMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DIAGONAL_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftMulDMemory::DftMulDMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftMulDMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout DftMulDMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& DftMulDMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftMulDMemory::set_diagonal(const vulkan::BufferWithMemory& diagonal) const
{
        ASSERT(diagonal.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = diagonal;
        buffer_info.offset = 0;
        buffer_info.range = diagonal.size();

        m_descriptors.update_descriptor_set(0, DIAGONAL_BINDING, buffer_info);
}

void DftMulDMemory::set_data(const vulkan::BufferWithMemory& data) const
{
        ASSERT(data.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = data;
        buffer_info.offset = 0;
        buffer_info.range = data.size();

        m_descriptors.update_descriptor_set(0, DATA_BINDING, buffer_info);
}

//

DftMulDConstant::DftMulDConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, group_size_x);
                entry.size = sizeof(Data::group_size_x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, group_size_y);
                entry.size = sizeof(Data::group_size_y);
                m_entries.push_back(entry);
        }

        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, rows);
                entry.size = sizeof(Data::rows);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, columns);
                entry.size = sizeof(Data::columns);
                m_entries.push_back(entry);
        }
}

void DftMulDConstant::set(uint32_t group_size_x, uint32_t group_size_y, int32_t rows, int32_t columns)
{
        static_assert(std::is_same_v<decltype(m_data.group_size_x), decltype(group_size_x)>);
        m_data.group_size_x = group_size_x;
        static_assert(std::is_same_v<decltype(m_data.group_size_y), decltype(group_size_y)>);
        m_data.group_size_y = group_size_y;
        static_assert(std::is_same_v<decltype(m_data.rows), decltype(rows)>);
        m_data.rows = rows;
        static_assert(std::is_same_v<decltype(m_data.columns), decltype(columns)>);
        m_data.columns = columns;
}

const std::vector<VkSpecializationMapEntry>& DftMulDConstant::entries() const
{
        return m_entries;
}

const void* DftMulDConstant::data() const
{
        return &m_data;
}

size_t DftMulDConstant::size() const
{
        return sizeof(m_data);
}
}
