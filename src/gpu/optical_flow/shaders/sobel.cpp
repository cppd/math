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

#include "sobel.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::optical_flow
{
std::vector<VkDescriptorSetLayoutBinding> SobelMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = I_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DX_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DY_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

SobelMemory::SobelMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned SobelMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& SobelMemory::descriptor_set(const int index) const
{
        ASSERT(index == 0 || index == 1);
        return descriptors_.descriptor_set(index);
}

void SobelMemory::set_i(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1)
{
        ASSERT(static_cast<VkImageView>(image_0) != static_cast<VkImageView>(image_1));
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        image_info.imageView = image_0;
        descriptors_.update_descriptor_set(0, I_BINDING, image_info);
        image_info.imageView = image_1;
        descriptors_.update_descriptor_set(1, I_BINDING, image_info);
}

void SobelMemory::set_dx(const vulkan::ImageView& image_dx)
{
        ASSERT(image_dx.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_dx.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image_dx;

        for (int s = 0; s < 2; ++s)
        {
                descriptors_.update_descriptor_set(s, DX_BINDING, image_info);
        }
}

void SobelMemory::set_dy(const vulkan::ImageView& image_dy)
{
        ASSERT(image_dy.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_dy.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image_dy;

        for (int s = 0; s < 2; ++s)
        {
                descriptors_.update_descriptor_set(s, DY_BINDING, image_info);
        }
}

//

SobelConstant::SobelConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, local_size_x);
                entry.size = sizeof(Data::local_size_x);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, local_size_y);
                entry.size = sizeof(Data::local_size_y);
                entries_.push_back(entry);
        }
}

void SobelConstant::set(const std::uint32_t local_size_x, const std::uint32_t local_size_y)
{
        static_assert(std::is_same_v<decltype(data_.local_size_x), std::remove_const_t<decltype(local_size_x)>>);
        data_.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(data_.local_size_y), std::remove_const_t<decltype(local_size_y)>>);
        data_.local_size_y = local_size_y;
}

const std::vector<VkSpecializationMapEntry>& SobelConstant::entries() const
{
        return entries_;
}

const void* SobelConstant::data() const
{
        return &data_;
}

std::size_t SobelConstant::size() const
{
        return sizeof(data_);
}

//

SobelProgram::SobelProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, SobelMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {SobelMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_sobel_comp(), "main")
{
}

VkDescriptorSetLayout SobelProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout SobelProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline SobelProgram::pipeline() const
{
        ASSERT(pipeline_ != VK_NULL_HANDLE);
        return pipeline_;
}

void SobelProgram::create_pipeline(const std::uint32_t local_size_x, const std::uint32_t local_size_y)
{
        constant_.set(local_size_x, local_size_y);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_;
        pipeline_ = create_compute_pipeline(info);
}

void SobelProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
