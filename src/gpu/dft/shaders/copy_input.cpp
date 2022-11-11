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

#include "copy_input.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline_compute.h>

namespace ns::gpu::dft
{
std::vector<VkDescriptorSetLayoutBinding> CopyInputMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SRC_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DST_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

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
        {
                ASSERT(input.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = input.handle();
                image_info.sampler = sampler;

                descriptors_.update_descriptor_set(0, SRC_BINDING, image_info);
        }
        {
                ASSERT(output.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = output.handle();
                buffer_info.offset = 0;
                buffer_info.range = output.size();

                descriptors_.update_descriptor_set(0, DST_BINDING, buffer_info);
        }
}

//

CopyInputConstant::CopyInputConstant()
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
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, x);
                entry.size = sizeof(Data::x);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, y);
                entry.size = sizeof(Data::y);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, width);
                entry.size = sizeof(Data::width);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, height);
                entry.size = sizeof(Data::height);
                entries_.push_back(entry);
        }
}

void CopyInputConstant::set(
        const std::int32_t local_size_x,
        const std::int32_t local_size_y,
        const Region<2, int>& rectangle)
{
        static_assert(std::is_same_v<decltype(data_.local_size_x), std::remove_const_t<decltype(local_size_x)>>);
        data_.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(data_.local_size_y), std::remove_const_t<decltype(local_size_y)>>);
        data_.local_size_y = local_size_y;

        ASSERT(rectangle.is_positive());
        data_.x = rectangle.x0();
        data_.y = rectangle.y0();
        data_.width = rectangle.width();
        data_.height = rectangle.height();
}

VkSpecializationInfo CopyInputConstant::info() const
{
        VkSpecializationInfo info = {};
        info.mapEntryCount = entries_.size();
        info.pMapEntries = entries_.data();
        info.dataSize = sizeof(data_);
        info.pData = &data_;
        return info;
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
        const VkSpecializationInfo constant_info = constant_.info();

        constant_.set(local_size_x, local_size_y, rectangle);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_info;
        pipeline_ = create_compute_pipeline(info);
}

void CopyInputProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
