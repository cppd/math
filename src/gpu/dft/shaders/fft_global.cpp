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

#include "fft_global.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline_compute.h>

namespace ns::gpu::dft
{
FftGlobalBuffer::FftGlobalBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : buffer_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Data))
{
}

const vulkan::Buffer& FftGlobalBuffer::buffer() const
{
        return buffer_.buffer();
}

void FftGlobalBuffer::set(const float two_pi_div_m, const int m_div_2) const
{
        Data d;
        d.two_pi_div_m = two_pi_div_m;
        d.m_div_2 = m_div_2;
        vulkan::map_and_write_to_buffer(buffer_, d);
}

//

std::vector<VkDescriptorSetLayoutBinding> FftGlobalMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

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

FftGlobalMemory::FftGlobalMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& data_buffer)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = data_buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = data_buffer.size();

        descriptors_.update_descriptor_set(0, DATA_BINDING, buffer_info);
}

unsigned FftGlobalMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& FftGlobalMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void FftGlobalMemory::set(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

FftGlobalConstant::FftGlobalConstant()
{
        entries_.resize(4);

        entries_[0].constantID = 0;
        entries_[0].offset = offsetof(Data, group_size);
        entries_[0].size = sizeof(Data::group_size);

        entries_[1].constantID = 1;
        entries_[1].offset = offsetof(Data, inverse);
        entries_[1].size = sizeof(Data::inverse);

        entries_[2].constantID = 2;
        entries_[2].offset = offsetof(Data, data_size);
        entries_[2].size = sizeof(Data::data_size);

        entries_[3].constantID = 3;
        entries_[3].offset = offsetof(Data, n);
        entries_[3].size = sizeof(Data::n);
}

void FftGlobalConstant::set(
        const std::uint32_t group_size,
        const bool inverse,
        const std::uint32_t data_size,
        const std::uint32_t n)
{
        data_ = {
                .group_size = group_size,
                .inverse = static_cast<std::uint32_t>(inverse ? 1 : 0),
                .data_size = data_size,
                .n = n};
}

VkSpecializationInfo FftGlobalConstant::info() const
{
        VkSpecializationInfo info = {};
        info.mapEntryCount = entries_.size();
        info.pMapEntries = entries_.data();
        info.dataSize = sizeof(data_);
        info.pData = &data_;
        return info;
}

//

FftGlobalProgram::FftGlobalProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, FftGlobalMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {FftGlobalMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_fft_global_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout FftGlobalProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout FftGlobalProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline FftGlobalProgram::pipeline(bool inverse) const
{
        if (inverse)
        {
                ASSERT(pipeline_inverse_ != VK_NULL_HANDLE);
                return pipeline_inverse_;
        }

        ASSERT(pipeline_forward_ != VK_NULL_HANDLE);
        return pipeline_forward_;
}

void FftGlobalProgram::create_pipelines(
        const std::uint32_t group_size,
        const std::uint32_t data_size,
        const std::uint32_t n)
{
        const VkSpecializationInfo constant_info = constant_.info();

        {
                constant_.set(group_size, false, data_size, n);

                vulkan::ComputePipelineCreateInfo info;
                info.device = device_;
                info.pipeline_layout = pipeline_layout_;
                info.shader = &shader_;
                info.constants = &constant_info;
                pipeline_forward_ = create_compute_pipeline(info);
        }
        {
                constant_.set(group_size, true, data_size, n);

                vulkan::ComputePipelineCreateInfo info;
                info.device = device_;
                info.pipeline_layout = pipeline_layout_;
                info.shader = &shader_;
                info.constants = &constant_info;
                pipeline_inverse_ = create_compute_pipeline(info);
        }
}

void FftGlobalProgram::delete_pipelines()
{
        pipeline_forward_ = vulkan::handle::Pipeline();
        pipeline_inverse_ = vulkan::handle::Pipeline();
}
}
