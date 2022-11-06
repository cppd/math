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

#include "flow.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::optical_flow
{
FlowDataBuffer::FlowDataBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : buffer_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(BufferData))
{
}

const vulkan::Buffer& FlowDataBuffer::buffer() const
{
        return buffer_.buffer();
}

void FlowDataBuffer::set(const Data& data) const
{
        BufferData buffer_data;
        buffer_data.point_count_x = data.point_count_x;
        buffer_data.point_count_y = data.point_count_y;
        buffer_data.use_all_points = data.use_all_points ? 1 : 0;
        buffer_data.use_guess = data.use_guess ? 1 : 0;
        buffer_data.guess_kx = data.guess_kx;
        buffer_data.guess_ky = data.guess_ky;
        buffer_data.guess_width = data.guess_width;
        vulkan::map_and_write_to_buffer(buffer_, buffer_data);
}

//

std::vector<VkDescriptorSetLayoutBinding> FlowMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TOP_POINTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINTS_FLOW_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINTS_FLOW_GUESS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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
                b.binding = J_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

FlowMemory::FlowMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& data_buffer)
        : descriptors_(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = data_buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = data_buffer.size();

        for (int i = 0; i < 2; ++i)
        {
                descriptors_.update_descriptor_set(i, DATA_BINDING, buffer_info);
        }
}

unsigned FlowMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& FlowMemory::descriptor_set(const int index) const
{
        ASSERT(index == 0 || index == 1);
        return descriptors_.descriptor_set(index);
}

void FlowMemory::set_dx(const vulkan::ImageView& image) const
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.handle();

        for (int s = 0; s < 2; ++s)
        {
                descriptors_.update_descriptor_set(s, DX_BINDING, image_info);
        }
}

void FlowMemory::set_dy(const vulkan::ImageView& image) const
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.handle();

        for (int s = 0; s < 2; ++s)
        {
                descriptors_.update_descriptor_set(s, DY_BINDING, image_info);
        }
}

void FlowMemory::set_i(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1) const
{
        ASSERT(image_0.handle() != image_1.handle());
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        image_info.imageView = image_0.handle();
        descriptors_.update_descriptor_set(0, I_BINDING, image_info);
        image_info.imageView = image_1.handle();
        descriptors_.update_descriptor_set(1, I_BINDING, image_info);
}

void FlowMemory::set_j(const VkSampler sampler, const vulkan::ImageView& image_0, const vulkan::ImageView& image_1)
        const
{
        ASSERT(image_0.handle() != image_1.handle());
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.sampler = sampler;

        image_info.imageView = image_0.handle();
        descriptors_.update_descriptor_set(0, J_BINDING, image_info);
        image_info.imageView = image_1.handle();
        descriptors_.update_descriptor_set(1, J_BINDING, image_info);
}

void FlowMemory::set_top_points(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        for (int s = 0; s < 2; ++s)
        {
                descriptors_.update_descriptor_set(s, TOP_POINTS_BINDING, buffer_info);
        }
}

void FlowMemory::set_flow(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        for (int s = 0; s < 2; ++s)
        {
                descriptors_.update_descriptor_set(s, POINTS_FLOW_BINDING, buffer_info);
        }
}

void FlowMemory::set_flow_guess(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        for (int s = 0; s < 2; ++s)
        {
                descriptors_.update_descriptor_set(s, POINTS_FLOW_GUESS_BINDING, buffer_info);
        }
}

//

FlowConstant::FlowConstant()
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
                entry.offset = offsetof(Data, radius);
                entry.size = sizeof(Data::radius);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, max_iteration_count);
                entry.size = sizeof(Data::max_iteration_count);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, stop_move_square);
                entry.size = sizeof(Data::stop_move_square);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, min_determinant);
                entry.size = sizeof(Data::min_determinant);
                entries_.push_back(entry);
        }
}

void FlowConstant::set(
        const std::uint32_t local_size_x,
        const std::uint32_t local_size_y,
        const std::int32_t radius,
        const std::int32_t max_iteration_count,
        const float stop_move_square,
        const float min_determinant)
{
        static_assert(std::is_same_v<decltype(data_.local_size_x), std::remove_const_t<decltype(local_size_x)>>);
        data_.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(data_.local_size_y), std::remove_const_t<decltype(local_size_y)>>);
        data_.local_size_y = local_size_y;
        static_assert(std::is_same_v<decltype(data_.radius), std::remove_const_t<decltype(radius)>>);
        data_.radius = radius;
        static_assert(std::is_same_v<
                      decltype(data_.max_iteration_count), std::remove_const_t<decltype(max_iteration_count)>>);
        data_.max_iteration_count = max_iteration_count;
        static_assert(
                std::is_same_v<decltype(data_.stop_move_square), std::remove_const_t<decltype(stop_move_square)>>);
        data_.stop_move_square = stop_move_square;
        static_assert(std::is_same_v<decltype(data_.min_determinant), std::remove_const_t<decltype(min_determinant)>>);
        data_.min_determinant = min_determinant;
}

VkSpecializationInfo FlowConstant::info() const
{
        VkSpecializationInfo info = {};
        info.mapEntryCount = entries_.size();
        info.pMapEntries = entries_.data();
        info.dataSize = sizeof(data_);
        info.pData = &data_;
        return info;
}

//

FlowProgram::FlowProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, FlowMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {FlowMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_flow_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout FlowProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout FlowProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline FlowProgram::pipeline() const
{
        ASSERT(pipeline_ != VK_NULL_HANDLE);
        return pipeline_;
}

void FlowProgram::create_pipeline(
        const std::uint32_t local_size_x,
        const std::uint32_t local_size_y,
        const std::int32_t radius,
        const std::int32_t max_iteration_count,
        const float stop_move_square,
        const float min_determinant)
{
        const VkSpecializationInfo constant_info = constant_.info();

        constant_.set(local_size_x, local_size_y, radius, max_iteration_count, stop_move_square, min_determinant);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_info;
        pipeline_ = create_compute_pipeline(info);
}

void FlowProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
