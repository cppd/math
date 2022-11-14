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

#include "grayscale.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline_compute.h>

#include <array>

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
                const std::uint32_t local_size_x,
                const std::uint32_t local_size_y,
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

std::vector<VkDescriptorSetLayoutBinding> GrayscaleMemory::descriptor_set_layout_bindings()
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
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

GrayscaleMemory::GrayscaleMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned GrayscaleMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& GrayscaleMemory::descriptor_set(const int index) const
{
        ASSERT(index == 0 || index == 1);
        return descriptors_.descriptor_set(index);
}

void GrayscaleMemory::set_src(const VkSampler sampler, const vulkan::ImageView& image)
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image.handle();
        image_info.sampler = sampler;

        for (int s = 0; s < 2; ++s)
        {
                descriptors_.update_descriptor_set(s, SRC_BINDING, image_info);
        }
}

void GrayscaleMemory::set_dst(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1)
{
        ASSERT(image_0.handle() != image_1.handle());
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        image_info.imageView = image_0.handle();
        descriptors_.update_descriptor_set(0, DST_BINDING, image_info);
        image_info.imageView = image_1.handle();
        descriptors_.update_descriptor_set(1, DST_BINDING, image_info);
}

//

GrayscaleProgram::GrayscaleProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, GrayscaleMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {GrayscaleMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_grayscale_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout GrayscaleProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout GrayscaleProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline GrayscaleProgram::pipeline() const
{
        ASSERT(pipeline_ != VK_NULL_HANDLE);
        return pipeline_;
}

void GrayscaleProgram::create_pipeline(
        const std::uint32_t local_size_x,
        const std::uint32_t local_size_y,
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

void GrayscaleProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
