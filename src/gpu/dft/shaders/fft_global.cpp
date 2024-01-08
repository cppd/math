/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/error.h>
#include <src/gpu/dft/code/code.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/compute.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::dft
{
namespace
{
class SpecializationConstants final
{
        struct Data final
        {
                std::uint32_t group_size;
                std::uint32_t inverse;
                std::uint32_t data_size;
                std::uint32_t n;
        };

        static constexpr std::array<VkSpecializationMapEntry, 4> ENTRIES{
                {{0, offsetof(Data, group_size), sizeof(Data::group_size)},
                 {1, offsetof(Data, inverse), sizeof(Data::inverse)},
                 {2, offsetof(Data, data_size), sizeof(Data::data_size)},
                 {3, offsetof(Data, n), sizeof(Data::n)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(const std::uint32_t group_size, const std::uint32_t data_size, const std::uint32_t n)
                : data_{.group_size = group_size, .inverse = 0, .data_size = data_size, .n = n}
        {
        }

        SpecializationConstants(const SpecializationConstants&) = delete;
        SpecializationConstants& operator=(const SpecializationConstants&) = delete;

        void set_inverse(const bool inverse)
        {
                data_.inverse = inverse ? 1 : 0;
        }

        [[nodiscard]] const VkSpecializationInfo& info() const
        {
                return info_;
        }
};
}

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
        bindings.reserve(2);

        bindings.push_back(
                {.binding = DATA_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = BUFFER_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

FftGlobalMemory::FftGlobalMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& buffer)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        descriptors_.update_descriptor_set(
                0, DATA_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
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

        descriptors_.update_descriptor_set(
                0, BUFFER_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
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
        SpecializationConstants constants(group_size, data_size, n);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();

        constants.set_inverse(false);
        pipeline_forward_ = create_compute_pipeline(info);

        constants.set_inverse(true);
        pipeline_inverse_ = create_compute_pipeline(info);
}

void FftGlobalProgram::delete_pipelines()
{
        pipeline_forward_ = vulkan::handle::Pipeline();
        pipeline_inverse_ = vulkan::handle::Pipeline();
}
}
