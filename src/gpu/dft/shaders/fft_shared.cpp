/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <src/vulkan/pipeline_compute.h>

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

FftSharedMemory::FftSharedMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
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

void FftSharedMemory::set(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
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
        const bool inverse,
        const std::uint32_t data_size,
        const std::uint32_t n,
        const std::uint32_t n_mask,
        const std::uint32_t n_bits,
        const std::uint32_t shared_size,
        const bool reverse_input,
        const std::uint32_t group_size)
{
        data_ = {
                .inverse = static_cast<std::uint32_t>(inverse ? 1 : 0),
                .data_size = data_size,
                .n = n,
                .n_mask = n_mask,
                .n_bits = n_bits,
                .shared_size = shared_size,
                .reverse_input = static_cast<std::uint32_t>(reverse_input ? 1 : 0),
                .group_size = group_size};
}

VkSpecializationInfo FftSharedConstant::info() const
{
        VkSpecializationInfo info = {};
        info.mapEntryCount = entries_.size();
        info.pMapEntries = entries_.data();
        info.dataSize = sizeof(data_);
        info.pData = &data_;
        return info;
}

//

FftSharedProgram::FftSharedProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, FftSharedMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {FftSharedMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_fft_shared_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
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

VkPipeline FftSharedProgram::pipeline(const bool inverse) const
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
        const std::uint32_t data_size,
        const std::uint32_t n,
        const std::uint32_t n_mask,
        const std::uint32_t n_bits,
        const std::uint32_t shared_size,
        const bool reverse_input,
        const std::uint32_t group_size)
{
        const VkSpecializationInfo constant_info = constant_.info();

        {
                constant_.set(false, data_size, n, n_mask, n_bits, shared_size, reverse_input, group_size);

                vulkan::ComputePipelineCreateInfo info;
                info.device = device_;
                info.pipeline_layout = pipeline_layout_;
                info.shader = &shader_;
                info.constants = &constant_info;
                pipeline_forward_ = create_compute_pipeline(info);
        }
        {
                constant_.set(true, data_size, n, n_mask, n_bits, shared_size, reverse_input, group_size);

                vulkan::ComputePipelineCreateInfo info;
                info.device = device_;
                info.pipeline_layout = pipeline_layout_;
                info.shader = &shader_;
                info.constants = &constant_info;
                pipeline_inverse_ = create_compute_pipeline(info);
        }
}

void FftSharedProgram::delete_pipelines()
{
        pipeline_forward_ = vulkan::handle::Pipeline();
        pipeline_inverse_ = vulkan::handle::Pipeline();
}
}
