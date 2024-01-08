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

#include "flow.h"

#include <src/com/error.h>
#include <src/gpu/optical_flow/code/code.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device/device.h>
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
                std::int32_t radius;
                std::int32_t max_iteration_count;
                float stop_move_square;
                float min_determinant;
        };

        static constexpr std::array<VkSpecializationMapEntry, 6> ENTRIES{
                {{0, offsetof(Data, local_size_x), sizeof(Data::local_size_x)},
                 {1, offsetof(Data, local_size_y), sizeof(Data::local_size_y)},
                 {2, offsetof(Data, radius), sizeof(Data::radius)},
                 {3, offsetof(Data, max_iteration_count), sizeof(Data::max_iteration_count)},
                 {4, offsetof(Data, stop_move_square), sizeof(Data::stop_move_square)},
                 {5, offsetof(Data, min_determinant), sizeof(Data::min_determinant)}}
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
                const std::int32_t radius,
                const std::int32_t max_iteration_count,
                const float stop_move_square,
                const float min_determinant)
                : data_{.local_size_x = local_size_x,
                        .local_size_y = local_size_y,
                        .radius = radius,
                        .max_iteration_count = max_iteration_count,
                        .stop_move_square = stop_move_square,
                        .min_determinant = min_determinant}
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
        bindings.reserve(8);

        bindings.push_back(
                {.binding = TOP_POINTS_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = POINTS_FLOW_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = POINTS_FLOW_GUESS_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = DATA_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = DX_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = DY_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = I_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = J_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

FlowMemory::FlowMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& data_buffer)
        : descriptors_(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        for (int i = 0; i < 2; ++i)
        {
                infos.emplace_back(
                        i, DATA_BINDING,
                        VkDescriptorBufferInfo{
                                .buffer = data_buffer.handle(),
                                .offset = 0,
                                .range = data_buffer.size()});
        }

        descriptors_.update_descriptor_set(infos);
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

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        for (int i = 0; i < 2; ++i)
        {
                infos.emplace_back(
                        i, DX_BINDING,
                        VkDescriptorImageInfo{
                                .sampler = VK_NULL_HANDLE,
                                .imageView = image.handle(),
                                .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
        }

        descriptors_.update_descriptor_set(infos);
}

void FlowMemory::set_dy(const vulkan::ImageView& image) const
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        for (int i = 0; i < 2; ++i)
        {
                infos.emplace_back(
                        i, DY_BINDING,
                        VkDescriptorImageInfo{
                                .sampler = VK_NULL_HANDLE,
                                .imageView = image.handle(),
                                .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
        }

        descriptors_.update_descriptor_set(infos);
}

void FlowMemory::set_i(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1) const
{
        ASSERT(image_0.handle() != image_1.handle());
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        infos.emplace_back(
                /*descriptor index*/ 0, I_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = image_0.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        infos.emplace_back(
                /*descriptor index*/ 1, I_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = image_1.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        descriptors_.update_descriptor_set(infos);
}

void FlowMemory::set_j(const VkSampler sampler, const vulkan::ImageView& image_0, const vulkan::ImageView& image_1)
        const
{
        ASSERT(image_0.handle() != image_1.handle());
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        infos.emplace_back(
                /*descriptor index*/ 0, J_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = image_0.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

        infos.emplace_back(
                /*descriptor index*/ 1, J_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = image_1.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

        descriptors_.update_descriptor_set(infos);
}

void FlowMemory::set_top_points(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        for (int i = 0; i < 2; ++i)
        {
                infos.emplace_back(
                        i, TOP_POINTS_BINDING,
                        VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
        }

        descriptors_.update_descriptor_set(infos);
}

void FlowMemory::set_flow(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        for (int i = 0; i < 2; ++i)
        {
                infos.emplace_back(
                        i, POINTS_FLOW_BINDING,
                        VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
        }

        descriptors_.update_descriptor_set(infos);
}

void FlowMemory::set_flow_guess(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        for (int i = 0; i < 2; ++i)
        {
                infos.emplace_back(
                        i, POINTS_FLOW_GUESS_BINDING,
                        VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
        }

        descriptors_.update_descriptor_set(infos);
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
        const SpecializationConstants constants(
                local_size_x, local_size_y, radius, max_iteration_count, stop_move_square, min_determinant);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();
        pipeline_ = create_compute_pipeline(info);
}

void FlowProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}
}
