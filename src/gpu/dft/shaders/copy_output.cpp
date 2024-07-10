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

#include "copy_output.h"

#include <src/com/error.h>
#include <src/gpu/dft/code/code.h>
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
                std::uint32_t local_size_x;
                std::uint32_t local_size_y;
                float to_mul;
        };

        static constexpr std::array<VkSpecializationMapEntry, 3> ENTRIES{
                {{0, offsetof(Data, local_size_x), sizeof(Data::local_size_x)},
                 {1, offsetof(Data, local_size_y), sizeof(Data::local_size_y)},
                 {2, offsetof(Data, to_mul), sizeof(Data::to_mul)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(const std::uint32_t local_size_x, const std::uint32_t local_size_y, const float to_mul)
                : data_{.local_size_x = local_size_x, .local_size_y = local_size_y, .to_mul = to_mul}
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

std::vector<VkDescriptorSetLayoutBinding> CopyOutputMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = SRC_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = DST_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

CopyOutputMemory::CopyOutputMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned CopyOutputMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& CopyOutputMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void CopyOutputMemory::set(const vulkan::Buffer& input, const vulkan::ImageView& output) const
{
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        ASSERT(input.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        infos.emplace_back(
                DESCRIPTOR_INDEX, SRC_BINDING,
                VkDescriptorBufferInfo{.buffer = input.handle(), .offset = 0, .range = input.size()});

        ASSERT(output.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(output.format() == VK_FORMAT_R32_SFLOAT);
        infos.emplace_back(
                DESCRIPTOR_INDEX, DST_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = output.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        descriptors_.update_descriptor_set(infos);
}

//

CopyOutputProgram::CopyOutputProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, CopyOutputMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {CopyOutputMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_copy_output_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout CopyOutputProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout CopyOutputProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline CopyOutputProgram::pipeline() const
{
        ASSERT(pipeline_ != VK_NULL_HANDLE);
        return pipeline_;
}

void CopyOutputProgram::create_pipeline(
        const std::uint32_t local_size_x,
        const std::uint32_t local_size_y,
        const float to_mul)
{
        const SpecializationConstants constants(local_size_x, local_size_y, to_mul);

        vulkan::pipeline::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();
        pipeline_ = vulkan::pipeline::create_compute_pipeline(info);
}

void CopyOutputProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
