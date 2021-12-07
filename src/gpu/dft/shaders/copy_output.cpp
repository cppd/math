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

#include "copy_output.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

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
                buffer_info.buffer = input;
                buffer_info.offset = 0;
                buffer_info.range = input.size();

                descriptors_.update_descriptor_set(0, SRC_BINDING, buffer_info);
        }
        {
                ASSERT(output.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
                ASSERT(output.format() == VK_FORMAT_R32_SFLOAT);

                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = output;

                descriptors_.update_descriptor_set(0, DST_BINDING, image_info);
        }
}

//

CopyOutputConstant::CopyOutputConstant()
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
                entry.offset = offsetof(Data, to_mul);
                entry.size = sizeof(Data::to_mul);
                entries_.push_back(entry);
        }
}

void CopyOutputConstant::set(const std::uint32_t local_size_x, const std::uint32_t local_size_y, const float to_mul)
{
        static_assert(std::is_same_v<decltype(data_.local_size_x), std::remove_const_t<decltype(local_size_x)>>);
        data_.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(data_.local_size_y), std::remove_const_t<decltype(local_size_y)>>);
        data_.local_size_y = local_size_y;
        static_assert(std::is_same_v<decltype(data_.to_mul), std::remove_const_t<decltype(to_mul)>>);
        data_.to_mul = to_mul;
}

const std::vector<VkSpecializationMapEntry>& CopyOutputConstant::entries() const
{
        return entries_;
}

const void* CopyOutputConstant::data() const
{
        return &data_;
}

std::size_t CopyOutputConstant::size() const
{
        return sizeof(data_);
}

//

CopyOutputProgram::CopyOutputProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, CopyOutputMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {CopyOutputMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_copy_output_comp(), "main")
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
        constant_.set(local_size_x, local_size_y, to_mul);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_;
        pipeline_ = create_compute_pipeline(info);
}

void CopyOutputProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
