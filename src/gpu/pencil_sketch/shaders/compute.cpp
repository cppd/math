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

#include "compute.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline_compute.h>

#include <array>

namespace ns::gpu::pencil_sketch
{
namespace
{
class SpecializationConstants final
{
        struct Data final
        {
                std::int32_t local_size;
                std::int32_t x;
                std::int32_t y;
                std::int32_t width;
                std::int32_t height;
        };

        static constexpr std::array<VkSpecializationMapEntry, 5> ENTRIES{
                {{0, offsetof(Data, local_size), sizeof(Data::local_size)},
                 {1, offsetof(Data, x), sizeof(Data::x)},
                 {2, offsetof(Data, y), sizeof(Data::y)},
                 {3, offsetof(Data, width), sizeof(Data::width)},
                 {4, offsetof(Data, height), sizeof(Data::height)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(const std::int32_t local_size, const Region<2, int>& rectangle)
                : data_{.local_size = local_size,
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

std::vector<VkDescriptorSetLayoutBinding> ComputeMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = INPUT_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OUTPUT_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OBJECTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

unsigned ComputeMemory::set_number()
{
        return SET_NUMBER;
}

ComputeMemory::ComputeMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

const VkDescriptorSet& ComputeMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void ComputeMemory::set_input(const VkSampler sampler, const vulkan::ImageView& image) const
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image.handle();
        image_info.sampler = sampler;

        descriptors_.update_descriptor_set(0, INPUT_BINDING, image_info);
}

void ComputeMemory::set_output_image(const vulkan::ImageView& image) const
{
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.handle();

        descriptors_.update_descriptor_set(0, OUTPUT_BINDING, image_info);
}

void ComputeMemory::set_object_image(const vulkan::ImageView& image) const
{
        ASSERT(image.format() == VK_FORMAT_R32_UINT);
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.handle();

        descriptors_.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

//

ComputeProgram::ComputeProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, ComputeMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {ComputeMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_compute_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout ComputeProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout ComputeProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline ComputeProgram::pipeline() const
{
        ASSERT(pipeline_ != VK_NULL_HANDLE);
        return pipeline_;
}

void ComputeProgram::create_pipeline(const unsigned group_size, const Region<2, int>& rectangle)
{
        const SpecializationConstants constants(group_size, rectangle);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();
        pipeline_ = create_compute_pipeline(info);
}

void ComputeProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
