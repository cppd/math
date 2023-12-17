/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/compute.h>

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
                std::uint32_t data_size;
                std::uint32_t n_mask;
                std::uint32_t n_bits;
        };

        static constexpr std::array<VkSpecializationMapEntry, 4> ENTRIES{
                {{0, offsetof(Data, group_size), sizeof(Data::group_size)},
                 {1, offsetof(Data, data_size), sizeof(Data::data_size)},
                 {2, offsetof(Data, n_mask), sizeof(Data::n_mask)},
                 {3, offsetof(Data, n_bits), sizeof(Data::n_bits)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(
                const std::uint32_t group_size,
                const std::uint32_t data_size,
                const std::uint32_t n_mask,
                const std::uint32_t n_bits)
                : data_{.group_size = group_size, .data_size = data_size, .n_mask = n_mask, .n_bits = n_bits}
        {
        }

        SpecializationConstants(const SpecializationConstants&) = delete;
        SpecializationConstants& operator=(const SpecializationConstants&) = delete;

        [[nodiscard]] const VkSpecializationInfo& info() const
        {
                return info_;
        }
};
}

std::vector<VkDescriptorSetLayoutBinding> BitReverseMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(1);

        bindings.push_back(
                {.binding = BUFFER_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

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

        descriptors_.update_descriptor_set(
                0, BUFFER_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
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
        const SpecializationConstants constants(group_size, data_size, n_mask, n_bits);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();
        pipeline_ = create_compute_pipeline(info);
}

void BitReverseProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
