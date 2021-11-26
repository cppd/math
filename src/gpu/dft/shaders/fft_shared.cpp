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

#include "fft_shared.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::dft
{
std::vector<VkDescriptorSetLayoutBinding> FftSharedMemory::descriptor_set_layout_bindings()
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

FftSharedMemory::FftSharedMemory(const VkDevice& device, VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned FftSharedMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& FftSharedMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void FftSharedMemory::set_buffer(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

FftSharedConstant::FftSharedConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, inverse);
                entry.size = sizeof(Data::inverse);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, data_size);
                entry.size = sizeof(Data::data_size);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, n);
                entry.size = sizeof(Data::n);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, n_mask);
                entry.size = sizeof(Data::n_mask);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, n_bits);
                entry.size = sizeof(Data::n_bits);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, shared_size);
                entry.size = sizeof(Data::shared_size);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 6;
                entry.offset = offsetof(Data, reverse_input);
                entry.size = sizeof(Data::reverse_input);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 7;
                entry.offset = offsetof(Data, group_size);
                entry.size = sizeof(Data::group_size);
                entries_.push_back(entry);
        }
}

void FftSharedConstant::set(
        bool inverse,
        uint32_t data_size,
        uint32_t n,
        uint32_t n_mask,
        uint32_t n_bits,
        uint32_t shared_size,
        bool reverse_input,
        uint32_t group_size)
{
        static_assert(std::is_same_v<decltype(data_.inverse), uint32_t>);
        data_.inverse = inverse ? 1 : 0;
        static_assert(std::is_same_v<decltype(data_.data_size), decltype(data_size)>);
        data_.data_size = data_size;
        static_assert(std::is_same_v<decltype(data_.n), decltype(n)>);
        data_.n = n;
        static_assert(std::is_same_v<decltype(data_.n_mask), decltype(n_mask)>);
        data_.n_mask = n_mask;
        static_assert(std::is_same_v<decltype(data_.n_bits), decltype(n_bits)>);
        data_.n_bits = n_bits;
        static_assert(std::is_same_v<decltype(data_.shared_size), decltype(shared_size)>);
        data_.shared_size = shared_size;
        static_assert(std::is_same_v<decltype(data_.reverse_input), uint32_t>);
        data_.reverse_input = reverse_input ? 1 : 0;
        static_assert(std::is_same_v<decltype(data_.group_size), decltype(group_size)>);
        data_.group_size = group_size;
}

const std::vector<VkSpecializationMapEntry>& FftSharedConstant::entries() const
{
        return entries_;
}

const void* FftSharedConstant::data() const
{
        return &data_;
}

std::size_t FftSharedConstant::size() const
{
        return sizeof(data_);
}

//

FftSharedProgram::FftSharedProgram(const VkDevice& device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, FftSharedMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {FftSharedMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_fft_shared_comp(), "main")
{
}

VkDescriptorSetLayout FftSharedProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout FftSharedProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline FftSharedProgram::pipeline(bool inverse) const
{
        if (inverse)
        {
                ASSERT(pipeline_inverse_ != VK_NULL_HANDLE);
                return pipeline_inverse_;
        }

        ASSERT(pipeline_forward_ != VK_NULL_HANDLE);
        return pipeline_forward_;
}

void FftSharedProgram::create_pipelines(
        uint32_t data_size,
        uint32_t n,
        uint32_t n_mask,
        uint32_t n_bits,
        uint32_t shared_size,
        bool reverse_input,
        uint32_t group_size)
{
        {
                constant_.set(false, data_size, n, n_mask, n_bits, shared_size, reverse_input, group_size);

                vulkan::ComputePipelineCreateInfo info;
                info.device = device_;
                info.pipeline_layout = pipeline_layout_;
                info.shader = &shader_;
                info.constants = &constant_;
                pipeline_forward_ = create_compute_pipeline(info);
        }
        {
                constant_.set(true, data_size, n, n_mask, n_bits, shared_size, reverse_input, group_size);

                vulkan::ComputePipelineCreateInfo info;
                info.device = device_;
                info.pipeline_layout = pipeline_layout_;
                info.shader = &shader_;
                info.constants = &constant_;
                pipeline_inverse_ = create_compute_pipeline(info);
        }
}

void FftSharedProgram::delete_pipelines()
{
        pipeline_forward_ = vulkan::handle::Pipeline();
        pipeline_inverse_ = vulkan::handle::Pipeline();
}
}
