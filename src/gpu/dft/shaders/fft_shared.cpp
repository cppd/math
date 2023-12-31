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

#include "fft_shared.h"

#include "../code/code.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
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
                std::uint32_t inverse;
                std::uint32_t data_size;
                std::uint32_t n;
                std::uint32_t n_mask;
                std::uint32_t n_bits;
                std::uint32_t shared_size;
                std::uint32_t reverse_input;
                std::uint32_t group_size;
        };

        static constexpr std::array<VkSpecializationMapEntry, 8> ENTRIES{
                {{0, offsetof(Data, inverse), sizeof(Data::inverse)},
                 {1, offsetof(Data, data_size), sizeof(Data::data_size)},
                 {2, offsetof(Data, n), sizeof(Data::n)},
                 {3, offsetof(Data, n_mask), sizeof(Data::n_mask)},
                 {4, offsetof(Data, n_bits), sizeof(Data::n_bits)},
                 {5, offsetof(Data, shared_size), sizeof(Data::shared_size)},
                 {6, offsetof(Data, reverse_input), sizeof(Data::reverse_input)},
                 {7, offsetof(Data, group_size), sizeof(Data::group_size)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(
                const std::uint32_t data_size,
                const std::uint32_t n,
                const std::uint32_t n_mask,
                const std::uint32_t n_bits,
                const std::uint32_t shared_size,
                const bool reverse_input,
                const std::uint32_t group_size)
                : data_{.inverse = 0,
                        .data_size = data_size,
                        .n = n,
                        .n_mask = n_mask,
                        .n_bits = n_bits,
                        .shared_size = shared_size,
                        .reverse_input = static_cast<std::uint32_t>(reverse_input ? 1 : 0),
                        .group_size = group_size}
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

std::vector<VkDescriptorSetLayoutBinding> FftSharedMemory::descriptor_set_layout_bindings()
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

        descriptors_.update_descriptor_set(
                0, BUFFER_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
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
        SpecializationConstants constants(data_size, n, n_mask, n_bits, shared_size, reverse_input, group_size);

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

void FftSharedProgram::delete_pipelines()
{
        pipeline_forward_ = vulkan::handle::Pipeline();
        pipeline_inverse_ = vulkan::handle::Pipeline();
}
}
