/*
Copyright (C) 2017-2021 Topological Manifold

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
#include <src/vulkan/pipeline.h>

namespace ns::gpu::pencil_sketch
{
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

ComputeMemory::ComputeMemory(const VkDevice& device, VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

const VkDescriptorSet& ComputeMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void ComputeMemory::set_input(VkSampler sampler, const vulkan::ImageView& image) const
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image;
        image_info.sampler = sampler;

        descriptors_.update_descriptor_set(0, INPUT_BINDING, image_info);
}

void ComputeMemory::set_output_image(const vulkan::ImageView& image) const
{
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image;

        descriptors_.update_descriptor_set(0, OUTPUT_BINDING, image_info);
}

void ComputeMemory::set_object_image(const vulkan::ImageView& image) const
{
        ASSERT(image.format() == VK_FORMAT_R32_UINT);
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image;

        descriptors_.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

//

ComputeConstant::ComputeConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, local_size);
                entry.size = sizeof(Data::local_size);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, x);
                entry.size = sizeof(Data::x);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, y);
                entry.size = sizeof(Data::y);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, width);
                entry.size = sizeof(Data::width);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, height);
                entry.size = sizeof(Data::height);
                entries_.push_back(entry);
        }
}

void ComputeConstant::set(int32_t local_size, const Region<2, int>& rectangle)
{
        static_assert(std::is_same_v<decltype(data_.local_size), decltype(local_size)>);
        data_.local_size = local_size;

        ASSERT(rectangle.is_positive());
        data_.x = rectangle.x0();
        data_.y = rectangle.y0();
        data_.width = rectangle.width();
        data_.height = rectangle.height();
}

const std::vector<VkSpecializationMapEntry>& ComputeConstant::entries() const
{
        return entries_;
}

const void* ComputeConstant::data() const
{
        return &data_;
}

std::size_t ComputeConstant::size() const
{
        return sizeof(data_);
}

//

ComputeProgram::ComputeProgram(const VkDevice& device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, ComputeMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {ComputeMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_compute_comp(), "main")
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

void ComputeProgram::create_pipeline(unsigned group_size, const Region<2, int>& rectangle)
{
        constant_.set(group_size, rectangle);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_;
        pipeline_ = create_compute_pipeline(info);
}

void ComputeProgram::delete_pipeline()
{
        pipeline_ = vulkan::Pipeline();
}
}
