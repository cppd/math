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

#include "downsample.h"

#include <src/com/error.h>
#include <src/gpu/optical_flow/code/code.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/compute.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::optical_flow
{
namespace
{
class SpecializationConstants final
{
        struct Data final
        {
                std::uint32_t local_size_x;
                std::uint32_t local_size_y;
        };

        static constexpr std::array<VkSpecializationMapEntry, 2> ENTRIES{
                {{0, offsetof(Data, local_size_x), sizeof(Data::local_size_x)},
                 {1, offsetof(Data, local_size_y), sizeof(Data::local_size_y)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(const std::uint32_t local_size_x, const std::uint32_t local_size_y)
                : data_{.local_size_x = local_size_x, .local_size_y = local_size_y}
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

std::vector<VkDescriptorSetLayoutBinding> DownsampleMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = BIG_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = SMALL_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

DownsampleMemory::DownsampleMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DownsampleMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& DownsampleMemory::descriptor_set(const int index) const
{
        ASSERT(index == 0 || index == 1);
        return descriptors_.descriptor_set(index);
}

void DownsampleMemory::set_big(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1) const
{
        ASSERT(image_0.handle() != image_1.handle());
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        infos.emplace_back(
                /*descriptor index*/ 0, BIG_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = image_0.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        infos.emplace_back(
                /*descriptor index*/ 1, BIG_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = image_1.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        descriptors_.update_descriptor_set(infos);
}

void DownsampleMemory::set_small(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1) const
{
        ASSERT(image_0.handle() != image_1.handle());
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        infos.emplace_back(
                /*descriptor index*/ 0, SMALL_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = image_0.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        infos.emplace_back(
                /*descriptor index*/ 1, SMALL_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = image_1.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        descriptors_.update_descriptor_set(infos);
}

//

DownsampleProgram::DownsampleProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, DownsampleMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {DownsampleMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_downsample_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout DownsampleProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout DownsampleProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline DownsampleProgram::pipeline() const
{
        ASSERT(pipeline_ != VK_NULL_HANDLE);
        return pipeline_;
}

void DownsampleProgram::create_pipeline(const std::uint32_t local_size_x, const std::uint32_t local_size_y)
{
        const SpecializationConstants constants(local_size_x, local_size_y);

        vulkan::pipeline::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();
        pipeline_ = vulkan::pipeline::create_compute_pipeline(info);
}

void DownsampleProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
