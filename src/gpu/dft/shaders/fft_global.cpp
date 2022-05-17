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
#include <src/vulkan/pipeline.h>

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
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, group_size);
                entry.size = sizeof(Data::group_size);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, inverse);
                entry.size = sizeof(Data::inverse);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, data_size);
                entry.size = sizeof(Data::data_size);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, n);
                entry.size = sizeof(Data::n);
                entries_.push_back(entry);
        }
}

void FftGlobalConstant::set(
        const std::uint32_t group_size,
        const bool inverse,
        const std::uint32_t data_size,
        const std::uint32_t n)
{
        static_assert(std::is_same_v<decltype(data_.group_size), std::remove_const_t<decltype(group_size)>>);
        data_.group_size = group_size;
        static_assert(std::is_same_v<decltype(data_.inverse), std::uint32_t>);
        data_.inverse = inverse ? 1 : 0;
        static_assert(std::is_same_v<decltype(data_.data_size), std::remove_const_t<decltype(data_size)>>);
        data_.data_size = data_size;
        static_assert(std::is_same_v<decltype(data_.n), std::remove_const_t<decltype(n)>>);
        data_.n = n;
}

const std::vector<VkSpecializationMapEntry>& FftGlobalConstant::entries() const
{
        return entries_;
}

const void* FftGlobalConstant::data() const
{
        return &data_;
}

std::size_t FftGlobalConstant::size() const
{
        return sizeof(data_);
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
        {
                constant_.set(group_size, false, data_size, n);

                vulkan::ComputePipelineCreateInfo info;
                info.device = device_;
                info.pipeline_layout = pipeline_layout_;
                info.shader = &shader_;
                info.constants = &constant_;
                pipeline_forward_ = create_compute_pipeline(info);
        }
        {
                constant_.set(group_size, true, data_size, n);

                vulkan::ComputePipelineCreateInfo info;
                info.device = device_;
                info.pipeline_layout = pipeline_layout_;
                info.shader = &shader_;
                info.constants = &constant_;
                pipeline_inverse_ = create_compute_pipeline(info);
        }
}

void FftGlobalProgram::delete_pipelines()
{
        pipeline_forward_ = vulkan::handle::Pipeline();
        pipeline_inverse_ = vulkan::handle::Pipeline();
}
}
