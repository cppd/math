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

#include "copy_input.h"

#include "../code/code.h"

#include <src/com/error.h>
#include <src/numerical/region.h>
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
                std::int32_t local_size_x;
                std::int32_t local_size_y;
                std::int32_t x;
                std::int32_t y;
                std::int32_t width;
                std::int32_t height;
        };

        static constexpr std::array<VkSpecializationMapEntry, 6> ENTRIES{
                {{0, offsetof(Data, local_size_x), sizeof(Data::local_size_x)},
                 {1, offsetof(Data, local_size_y), sizeof(Data::local_size_y)},
                 {2, offsetof(Data, x), sizeof(Data::x)},
                 {3, offsetof(Data, y), sizeof(Data::y)},
                 {4, offsetof(Data, width), sizeof(Data::width)},
                 {5, offsetof(Data, height), sizeof(Data::height)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(
                const std::int32_t local_size_x,
                const std::int32_t local_size_y,
                const Region<2, int>& rectangle)
                : data_{.local_size_x = local_size_x,
                        .local_size_y = local_size_y,
                        .x = rectangle.x0(),
                        .y = rectangle.y0(),
                        .width = rectangle.width(),
                        .height = rectangle.height()}
        {
                ASSERT(rectangle.is_positive());
        }

        SpecializationConstants(const SpecializationConstants&) = delete;
        SpecializationConstants& operator=(const SpecializationConstants&) = delete;

        [[nodiscard]] const VkSpecializationInfo& info() const
        {
                return info_;
        }
};
}

std::vector<VkDescriptorSetLayoutBinding> CopyInputMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = SRC_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = DST_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

CopyInputMemory::CopyInputMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned CopyInputMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& CopyInputMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void CopyInputMemory::set(const VkSampler sampler, const vulkan::ImageView& input, const vulkan::Buffer& output) const
{
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        ASSERT(input.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        infos.emplace_back(
                DESCRIPTOR_INDEX, SRC_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = input.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

        ASSERT(output.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        infos.emplace_back(
                DESCRIPTOR_INDEX, DST_BINDING,
                VkDescriptorBufferInfo{.buffer = output.handle(), .offset = 0, .range = output.size()});

        descriptors_.update_descriptor_set(infos);
}

//

CopyInputProgram::CopyInputProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, CopyInputMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {CopyInputMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_copy_input_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout CopyInputProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout CopyInputProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline CopyInputProgram::pipeline() const
{
        ASSERT(pipeline_ != VK_NULL_HANDLE);
        return pipeline_;
}

void CopyInputProgram::create_pipeline(
        const std::int32_t local_size_x,
        const std::int32_t local_size_y,
        const Region<2, int>& rectangle)
{
        const SpecializationConstants constants(local_size_x, local_size_y, rectangle);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();
        pipeline_ = create_compute_pipeline(info);
}

void CopyInputProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
