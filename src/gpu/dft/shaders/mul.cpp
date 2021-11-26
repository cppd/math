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

#include "mul.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::dft
{
std::vector<VkDescriptorSetLayoutBinding> MulMemory::descriptor_set_layout_bindings()
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

MulMemory::MulMemory(const VkDevice& device, VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned MulMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& MulMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void MulMemory::set(const vulkan::Buffer& data, const vulkan::Buffer& buffer) const
{
        {
                ASSERT(data.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = data;
                buffer_info.offset = 0;
                buffer_info.range = data.size();

                descriptors_.update_descriptor_set(0, DATA_BINDING, buffer_info);
        }
        {
                ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffer;
                buffer_info.offset = 0;
                buffer_info.range = buffer.size();

                descriptors_.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
        }
}

//

MulConstant::MulConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, function_index);
                entry.size = sizeof(Data::function_index);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, n1);
                entry.size = sizeof(Data::n1);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, n2);
                entry.size = sizeof(Data::n2);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, m1);
                entry.size = sizeof(Data::m1);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, m2);
                entry.size = sizeof(Data::m2);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, inverse);
                entry.size = sizeof(Data::inverse);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 6;
                entry.offset = offsetof(Data, group_size_x);
                entry.size = sizeof(Data::group_size_x);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 7;
                entry.offset = offsetof(Data, group_size_y);
                entry.size = sizeof(Data::group_size_y);
                entries_.push_back(entry);
        }
}

void MulConstant::set_data(int32_t n1, int32_t n2, int32_t m1, int32_t m2, uint32_t group_size_x, uint32_t group_size_y)
{
        static_assert(std::is_same_v<decltype(data_.n1), decltype(n1)>);
        data_.n1 = n1;
        static_assert(std::is_same_v<decltype(data_.n2), decltype(n2)>);
        data_.n2 = n2;
        static_assert(std::is_same_v<decltype(data_.m1), decltype(m1)>);
        data_.m1 = m1;
        static_assert(std::is_same_v<decltype(data_.m2), decltype(m2)>);
        data_.m2 = m2;
        static_assert(std::is_same_v<decltype(data_.group_size_x), decltype(group_size_x)>);
        data_.group_size_x = group_size_x;
        static_assert(std::is_same_v<decltype(data_.group_size_y), decltype(group_size_y)>);
        data_.group_size_y = group_size_y;
}

void MulConstant::set_function(int32_t function_index, bool inverse)
{
        static_assert(std::is_same_v<decltype(data_.function_index), int32_t>);
        data_.function_index = function_index;
        static_assert(std::is_same_v<decltype(data_.inverse), uint32_t>);
        data_.inverse = inverse ? 1 : 0;
}

const std::vector<VkSpecializationMapEntry>& MulConstant::entries() const
{
        return entries_;
}

const void* MulConstant::data() const
{
        return &data_;
}

std::size_t MulConstant::size() const
{
        return sizeof(data_);
}

//

MulProgram::MulProgram(const VkDevice& device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, MulMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(device, {MulMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_mul_comp(), "main")
{
}

VkDescriptorSetLayout MulProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout MulProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline MulProgram::pipeline_rows_to_buffer(bool inverse) const
{
        if (!inverse)
        {
                ASSERT(pipeline_rows_to_buffer_forward_ != VK_NULL_HANDLE);
                return pipeline_rows_to_buffer_forward_;
        }

        ASSERT(pipeline_rows_to_buffer_inverse_ != VK_NULL_HANDLE);
        return pipeline_rows_to_buffer_inverse_;
}

VkPipeline MulProgram::pipeline_rows_from_buffer(bool inverse) const
{
        if (!inverse)
        {
                ASSERT(pipeline_rows_from_buffer_forward_ != VK_NULL_HANDLE);
                return pipeline_rows_from_buffer_forward_;
        }

        ASSERT(pipeline_rows_from_buffer_inverse_ != VK_NULL_HANDLE);
        return pipeline_rows_from_buffer_inverse_;
}

VkPipeline MulProgram::pipeline_columns_to_buffer(bool inverse) const
{
        if (!inverse)
        {
                ASSERT(pipeline_columns_to_buffer_forward_ != VK_NULL_HANDLE);
                return pipeline_columns_to_buffer_forward_;
        }

        ASSERT(pipeline_columns_to_buffer_inverse_ != VK_NULL_HANDLE);
        return pipeline_columns_to_buffer_inverse_;
}

VkPipeline MulProgram::pipeline_columns_from_buffer(bool inverse) const
{
        if (!inverse)
        {
                ASSERT(pipeline_columns_from_buffer_forward_ != VK_NULL_HANDLE);
                return pipeline_columns_from_buffer_forward_;
        }

        ASSERT(pipeline_columns_from_buffer_inverse_ != VK_NULL_HANDLE);
        return pipeline_columns_from_buffer_inverse_;
}

void MulProgram::create_pipelines(
        int32_t n1,
        int32_t n2,
        int32_t m1,
        int32_t m2,
        uint32_t group_size_x,
        uint32_t group_size_y)
{
        constant_.set_data(n1, n2, m1, m2, group_size_x, group_size_y);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_;

        constant_.set_function(0, false);
        pipeline_rows_to_buffer_forward_ = create_compute_pipeline(info);
        constant_.set_function(0, true);
        pipeline_rows_to_buffer_inverse_ = create_compute_pipeline(info);

        constant_.set_function(1, false);
        pipeline_rows_from_buffer_forward_ = create_compute_pipeline(info);
        constant_.set_function(1, true);
        pipeline_rows_from_buffer_inverse_ = create_compute_pipeline(info);

        constant_.set_function(2, false);
        pipeline_columns_to_buffer_forward_ = create_compute_pipeline(info);
        constant_.set_function(2, true);
        pipeline_columns_to_buffer_inverse_ = create_compute_pipeline(info);

        constant_.set_function(3, false);
        pipeline_columns_from_buffer_forward_ = create_compute_pipeline(info);
        constant_.set_function(3, true);
        pipeline_columns_from_buffer_inverse_ = create_compute_pipeline(info);
}

void MulProgram::delete_pipelines()
{
        pipeline_rows_to_buffer_forward_ = vulkan::handle::Pipeline();
        pipeline_rows_to_buffer_inverse_ = vulkan::handle::Pipeline();
        pipeline_rows_from_buffer_forward_ = vulkan::handle::Pipeline();
        pipeline_rows_from_buffer_inverse_ = vulkan::handle::Pipeline();
        pipeline_columns_to_buffer_forward_ = vulkan::handle::Pipeline();
        pipeline_columns_to_buffer_inverse_ = vulkan::handle::Pipeline();
        pipeline_columns_from_buffer_forward_ = vulkan::handle::Pipeline();
        pipeline_columns_from_buffer_inverse_ = vulkan::handle::Pipeline();
}
}
