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

#include "compute_mul.h"

#include "shader_source.h"

#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> DftMulMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

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

DftMulMemory::DftMulMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftMulMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& DftMulMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftMulMemory::set(const vulkan::BufferWithMemory& data, const vulkan::BufferWithMemory& buffer) const
{
        {
                ASSERT(data.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = data;
                buffer_info.offset = 0;
                buffer_info.range = data.size();

                m_descriptors.update_descriptor_set(0, DATA_BINDING, buffer_info);
        }
        {
                ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffer;
                buffer_info.offset = 0;
                buffer_info.range = buffer.size();

                m_descriptors.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
        }
}

//

DftMulConstant::DftMulConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, function_index);
                entry.size = sizeof(Data::function_index);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, n1);
                entry.size = sizeof(Data::n1);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, n2);
                entry.size = sizeof(Data::n2);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, m1);
                entry.size = sizeof(Data::m1);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, m2);
                entry.size = sizeof(Data::m2);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, inverse);
                entry.size = sizeof(Data::inverse);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 6;
                entry.offset = offsetof(Data, group_size_x);
                entry.size = sizeof(Data::group_size_x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 7;
                entry.offset = offsetof(Data, group_size_y);
                entry.size = sizeof(Data::group_size_y);
                m_entries.push_back(entry);
        }
}

void DftMulConstant::set_data(int32_t n1, int32_t n2, int32_t m1, int32_t m2, uint32_t group_size_x, uint32_t group_size_y)
{
        static_assert(std::is_same_v<decltype(m_data.n1), decltype(n1)>);
        m_data.n1 = n1;
        static_assert(std::is_same_v<decltype(m_data.n2), decltype(n2)>);
        m_data.n2 = n2;
        static_assert(std::is_same_v<decltype(m_data.m1), decltype(m1)>);
        m_data.m1 = m1;
        static_assert(std::is_same_v<decltype(m_data.m2), decltype(m2)>);
        m_data.m2 = m2;
        static_assert(std::is_same_v<decltype(m_data.group_size_x), decltype(group_size_x)>);
        m_data.group_size_x = group_size_x;
        static_assert(std::is_same_v<decltype(m_data.group_size_y), decltype(group_size_y)>);
        m_data.group_size_y = group_size_y;
}

void DftMulConstant::set_function(int32_t function_index, bool inverse)
{
        static_assert(std::is_same_v<decltype(m_data.function_index), int32_t>);
        m_data.function_index = function_index;
        static_assert(std::is_same_v<decltype(m_data.inverse), uint32_t>);
        m_data.inverse = inverse ? 1 : 0;
}

const std::vector<VkSpecializationMapEntry>& DftMulConstant::entries() const
{
        return m_entries;
}

const void* DftMulConstant::data() const
{
        return &m_data;
}

size_t DftMulConstant::size() const
{
        return sizeof(m_data);
}

//

DftMulProgram::DftMulProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, DftMulMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(device, {DftMulMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, dft_mul_comp(), "main")
{
}

VkDescriptorSetLayout DftMulProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout DftMulProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline DftMulProgram::pipeline_rows_to_buffer(bool inverse) const
{
        if (!inverse)
        {
                ASSERT(m_pipeline_rows_to_buffer_forward != VK_NULL_HANDLE);
                return m_pipeline_rows_to_buffer_forward;
        }
        else
        {
                ASSERT(m_pipeline_rows_to_buffer_inverse != VK_NULL_HANDLE);
                return m_pipeline_rows_to_buffer_inverse;
        }
}

VkPipeline DftMulProgram::pipeline_rows_from_buffer(bool inverse) const
{
        if (!inverse)
        {
                ASSERT(m_pipeline_rows_from_buffer_forward != VK_NULL_HANDLE);
                return m_pipeline_rows_from_buffer_forward;
        }
        else
        {
                ASSERT(m_pipeline_rows_from_buffer_inverse != VK_NULL_HANDLE);
                return m_pipeline_rows_from_buffer_inverse;
        }
}

VkPipeline DftMulProgram::pipeline_columns_to_buffer(bool inverse) const
{
        if (!inverse)
        {
                ASSERT(m_pipeline_columns_to_buffer_forward != VK_NULL_HANDLE);
                return m_pipeline_columns_to_buffer_forward;
        }
        else
        {
                ASSERT(m_pipeline_columns_to_buffer_inverse != VK_NULL_HANDLE);
                return m_pipeline_columns_to_buffer_inverse;
        }
}

VkPipeline DftMulProgram::pipeline_columns_from_buffer(bool inverse) const
{
        if (!inverse)
        {
                ASSERT(m_pipeline_columns_from_buffer_forward != VK_NULL_HANDLE);
                return m_pipeline_columns_from_buffer_forward;
        }
        else
        {
                ASSERT(m_pipeline_columns_from_buffer_inverse != VK_NULL_HANDLE);
                return m_pipeline_columns_from_buffer_inverse;
        }
}

void DftMulProgram::create_pipelines(int32_t n1, int32_t n2, int32_t m1, int32_t m2, uint32_t group_size_x, uint32_t group_size_y)
{
        m_constant.set_data(n1, n2, m1, m2, group_size_x, group_size_y);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;

        m_constant.set_function(0, false);
        m_pipeline_rows_to_buffer_forward = create_compute_pipeline(info);
        m_constant.set_function(0, true);
        m_pipeline_rows_to_buffer_inverse = create_compute_pipeline(info);

        m_constant.set_function(1, false);
        m_pipeline_rows_from_buffer_forward = create_compute_pipeline(info);
        m_constant.set_function(1, true);
        m_pipeline_rows_from_buffer_inverse = create_compute_pipeline(info);

        m_constant.set_function(2, false);
        m_pipeline_columns_to_buffer_forward = create_compute_pipeline(info);
        m_constant.set_function(2, true);
        m_pipeline_columns_to_buffer_inverse = create_compute_pipeline(info);

        m_constant.set_function(3, false);
        m_pipeline_columns_from_buffer_forward = create_compute_pipeline(info);
        m_constant.set_function(3, true);
        m_pipeline_columns_from_buffer_inverse = create_compute_pipeline(info);
}

void DftMulProgram::delete_pipelines()
{
        m_pipeline_rows_to_buffer_forward = vulkan::Pipeline();
        m_pipeline_rows_to_buffer_inverse = vulkan::Pipeline();
        m_pipeline_rows_from_buffer_forward = vulkan::Pipeline();
        m_pipeline_rows_from_buffer_inverse = vulkan::Pipeline();
        m_pipeline_columns_to_buffer_forward = vulkan::Pipeline();
        m_pipeline_columns_to_buffer_inverse = vulkan::Pipeline();
        m_pipeline_columns_from_buffer_forward = vulkan::Pipeline();
        m_pipeline_columns_from_buffer_inverse = vulkan::Pipeline();
}
}
