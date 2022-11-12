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

#include "copy_output.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline_compute.h>

namespace ns::gpu::dft
{
std::vector<VkDescriptorSetLayoutBinding> CopyOutputMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SRC_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DST_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

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
        {
                ASSERT(input.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = input.handle();
                buffer_info.offset = 0;
                buffer_info.range = input.size();

                descriptors_.update_descriptor_set(0, SRC_BINDING, buffer_info);
        }
        {
                ASSERT(output.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
                ASSERT(output.format() == VK_FORMAT_R32_SFLOAT);

                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = output.handle();

                descriptors_.update_descriptor_set(0, DST_BINDING, image_info);
        }
}

//

CopyOutputConstant::CopyOutputConstant()
{
        entries_.resize(3);

        entries_[0].constantID = 0;
        entries_[0].offset = offsetof(Data, local_size_x);
        entries_[0].size = sizeof(Data::local_size_x);

        entries_[1].constantID = 1;
        entries_[1].offset = offsetof(Data, local_size_y);
        entries_[1].size = sizeof(Data::local_size_y);

        entries_[2].constantID = 2;
        entries_[2].offset = offsetof(Data, to_mul);
        entries_[2].size = sizeof(Data::to_mul);
}

void CopyOutputConstant::set(const std::uint32_t local_size_x, const std::uint32_t local_size_y, const float to_mul)
{
        data_ = {.local_size_x = local_size_x, .local_size_y = local_size_y, .to_mul = to_mul};
}

VkSpecializationInfo CopyOutputConstant::info() const
{
        VkSpecializationInfo info = {};
        info.mapEntryCount = entries_.size();
        info.pMapEntries = entries_.data();
        info.dataSize = sizeof(data_);
        info.pData = &data_;
        return info;
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
        const VkSpecializationInfo constant_info = constant_.info();

        constant_.set(local_size_x, local_size_y, to_mul);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_info;
        pipeline_ = create_compute_pipeline(info);
}

void CopyOutputProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
