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

#include "bit_reverse.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline_compute.h>

namespace ns::gpu::dft
{
std::vector<VkDescriptorSetLayoutBinding> BitReverseMemory::descriptor_set_layout_bindings()
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

BitReverseMemory::BitReverseMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned BitReverseMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& BitReverseMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void BitReverseMemory::set(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

BitReverseConstant::BitReverseConstant()
{
        entries_.resize(4);

        entries_[0].constantID = 0;
        entries_[0].offset = offsetof(Data, group_size);
        entries_[0].size = sizeof(Data::group_size);

        entries_[1].constantID = 1;
        entries_[1].offset = offsetof(Data, data_size);
        entries_[1].size = sizeof(Data::data_size);

        entries_[2].constantID = 2;
        entries_[2].offset = offsetof(Data, n_mask);
        entries_[2].size = sizeof(Data::n_mask);

        entries_[3].constantID = 3;
        entries_[3].offset = offsetof(Data, n_bits);
        entries_[3].size = sizeof(Data::n_bits);
}

void BitReverseConstant::set(
        const std::uint32_t group_size,
        const std::uint32_t data_size,
        const std::uint32_t n_mask,
        const std::uint32_t n_bits)
{
        data_ = {.group_size = group_size, .data_size = data_size, .n_mask = n_mask, .n_bits = n_bits};
}

VkSpecializationInfo BitReverseConstant::info() const
{
        VkSpecializationInfo info = {};
        info.mapEntryCount = entries_.size();
        info.pMapEntries = entries_.data();
        info.dataSize = sizeof(data_);
        info.pData = &data_;
        return info;
}

//

BitReverseProgram::BitReverseProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, BitReverseMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {BitReverseMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_bit_reverse_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout BitReverseProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout BitReverseProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline BitReverseProgram::pipeline() const
{
        ASSERT(pipeline_ != VK_NULL_HANDLE);
        return pipeline_;
}

void BitReverseProgram::create_pipeline(
        const std::uint32_t group_size,
        const std::uint32_t data_size,
        const std::uint32_t n_mask,
        const std::uint32_t n_bits)
{
        const VkSpecializationInfo constant_info = constant_.info();

        constant_.set(group_size, data_size, n_mask, n_bits);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_info;
        pipeline_ = create_compute_pipeline(info);
}

void BitReverseProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
