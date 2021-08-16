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

#include "prepare.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

#include <type_traits>

namespace ns::gpu::convex_hull
{
std::vector<VkDescriptorSetLayoutBinding> PrepareMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OBJECTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = LINES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

PrepareMemory::PrepareMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned PrepareMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& PrepareMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void PrepareMemory::set_object_image(const vulkan::ImageWithMemory& storage_image) const
{
        ASSERT(storage_image.format() == VK_FORMAT_R32_UINT);
        ASSERT(storage_image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = storage_image.image_view();

        descriptors_.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

void PrepareMemory::set_lines(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, LINES_BINDING, buffer_info);
}

//

PrepareConstant::PrepareConstant()
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
                entry.offset = offsetof(Data, buffer_size);
                entry.size = sizeof(Data::buffer_size);
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

void PrepareConstant::set(int32_t local_size_x, int32_t buffer_size, const Region<2, int>& rectangle)
{
        static_assert(std::is_same_v<decltype(data_.local_size_x), decltype(local_size_x)>);
        data_.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(data_.buffer_size), decltype(buffer_size)>);
        data_.buffer_size = buffer_size;

        ASSERT(rectangle.is_positive());
        data_.x = rectangle.x0();
        data_.y = rectangle.y0();
        data_.width = rectangle.width();
        data_.height = rectangle.height();
}

const std::vector<VkSpecializationMapEntry>& PrepareConstant::entries() const
{
        return entries_;
}

const void* PrepareConstant::data() const
{
        return &data_;
}

std::size_t PrepareConstant::size() const
{
        return sizeof(data_);
}

//

PrepareProgram::PrepareProgram(const vulkan::Device& device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, PrepareMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {PrepareMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_prepare_comp(), "main")
{
}

void PrepareProgram::create_pipeline(unsigned buffer_and_group_size, const Region<2, int>& rectangle)
{
        constant_.set(buffer_and_group_size, buffer_and_group_size, rectangle);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_;
        pipeline_ = create_compute_pipeline(info);
}

void PrepareProgram::delete_pipeline()
{
        pipeline_ = vulkan::Pipeline();
}

VkDescriptorSetLayout PrepareProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout PrepareProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline PrepareProgram::pipeline() const
{
        return pipeline_;
}
}
