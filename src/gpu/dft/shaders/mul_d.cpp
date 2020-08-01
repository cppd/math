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

#include "mul_d.h"

#include "code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu::dft
{
std::vector<VkDescriptorSetLayoutBinding> MulDMemory::descriptor_set_layout_bindings()
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

MulDMemory::MulDMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned MulDMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& MulDMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void MulDMemory::set(const vulkan::BufferWithMemory& diagonal, const vulkan::BufferWithMemory& data) const
{
        {
                ASSERT(diagonal.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = diagonal;
                buffer_info.offset = 0;
                buffer_info.range = diagonal.size();

                m_descriptors.update_descriptor_set(0, DIAGONAL_BINDING, buffer_info);
        }
        {
                ASSERT(data.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = data;
                buffer_info.offset = 0;
                buffer_info.range = data.size();

                m_descriptors.update_descriptor_set(0, DATA_BINDING, buffer_info);
        }
}

//

MulDConstant::MulDConstant()
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

void MulDConstant::set(uint32_t group_size_x, uint32_t group_size_y, int32_t rows, int32_t columns)
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

const std::vector<VkSpecializationMapEntry>& MulDConstant::entries() const
{
        return m_entries;
}

const void* MulDConstant::data() const
{
        return &m_data;
}

size_t MulDConstant::size() const
{
        return sizeof(m_data);
}

//

MulDProgram::MulDProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, MulDMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {MulDMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, code_mul_d_comp(), "main")
{
}

VkDescriptorSetLayout MulDProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout MulDProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline MulDProgram::pipeline_rows() const
{
        ASSERT(m_pipeline_rows != VK_NULL_HANDLE);
        return m_pipeline_rows;
}

VkPipeline MulDProgram::pipeline_columns() const
{
        ASSERT(m_pipeline_columns != VK_NULL_HANDLE);
        return m_pipeline_columns;
}

void MulDProgram::create_pipelines(
        uint32_t n1,
        uint32_t n2,
        uint32_t m1,
        uint32_t m2,
        uint32_t group_size_x,
        uint32_t group_size_y)
{
        {
                m_constant.set(group_size_x, group_size_y, n2, m1);

                vulkan::ComputePipelineCreateInfo info;
                info.device = &m_device;
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_shader;
                info.constants = &m_constant;
                m_pipeline_rows = create_compute_pipeline(info);
        }
        {
                m_constant.set(group_size_x, group_size_y, n1, m2);

                vulkan::ComputePipelineCreateInfo info;
                info.device = &m_device;
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_shader;
                info.constants = &m_constant;
                m_pipeline_columns = create_compute_pipeline(info);
        }
}

void MulDProgram::delete_pipelines()
{
        m_pipeline_rows = vulkan::Pipeline();
        m_pipeline_columns = vulkan::Pipeline();
}
}
