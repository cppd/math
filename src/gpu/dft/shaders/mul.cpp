/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "mul.h"

#include "../code/code.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/compute.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::dft
{
namespace
{
class SpecializationConstants final
{
        struct Data final
        {
                std::int32_t function_index;
                std::uint32_t inverse;
                std::int32_t n_1;
                std::int32_t n_2;
                std::int32_t m_1;
                std::int32_t m_2;
                std::uint32_t group_size_x;
                std::uint32_t group_size_y;
        };

        static constexpr std::array<VkSpecializationMapEntry, 8> ENTRIES{
                {{0, offsetof(Data, function_index), sizeof(Data::function_index)},
                 {1, offsetof(Data, inverse), sizeof(Data::inverse)},
                 {2, offsetof(Data, n_1), sizeof(Data::n_1)},
                 {3, offsetof(Data, n_2), sizeof(Data::n_2)},
                 {4, offsetof(Data, m_1), sizeof(Data::m_1)},
                 {5, offsetof(Data, m_2), sizeof(Data::m_2)},
                 {6, offsetof(Data, group_size_x), sizeof(Data::group_size_x)},
                 {7, offsetof(Data, group_size_y), sizeof(Data::group_size_y)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(
                const std::int32_t n_1,
                const std::int32_t n_2,
                const std::int32_t m_1,
                const std::int32_t m_2,
                const std::uint32_t group_size_x,
                const std::uint32_t group_size_y)
                : data_{.function_index = 0,
                        .inverse = 0,
                        .n_1 = n_1,
                        .n_2 = n_2,
                        .m_1 = m_1,
                        .m_2 = m_2,
                        .group_size_x = group_size_x,
                        .group_size_y = group_size_y}
        {
        }

        SpecializationConstants(const SpecializationConstants&) = delete;
        SpecializationConstants& operator=(const SpecializationConstants&) = delete;

        void set_function(const std::int32_t function_index, const bool inverse)
        {
                data_.function_index = function_index;
                data_.inverse = inverse ? 1 : 0;
        }

        [[nodiscard]] const VkSpecializationInfo& info() const
        {
                return info_;
        }
};
}

std::vector<VkDescriptorSetLayoutBinding> MulMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = DATA_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = BUFFER_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

MulMemory::MulMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned MulMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& MulMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void MulMemory::set(const vulkan::Buffer& data, const vulkan::Buffer& buffer) const
{
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        ASSERT(data.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        infos.emplace_back(
                DESCRIPTOR_INDEX, DATA_BINDING,
                VkDescriptorBufferInfo{.buffer = data.handle(), .offset = 0, .range = data.size()});

        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        infos.emplace_back(
                DESCRIPTOR_INDEX, BUFFER_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});

        descriptors_.update_descriptor_set(infos);
}

//

MulProgram::MulProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, MulMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(device, {MulMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_mul_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout MulProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout MulProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline MulProgram::pipeline_rows_to_buffer(const bool inverse) const
{
        if (!inverse)
        {
                ASSERT(pipeline_rows_to_buffer_forward_ != VK_NULL_HANDLE);
                return pipeline_rows_to_buffer_forward_;
        }

        ASSERT(pipeline_rows_to_buffer_inverse_ != VK_NULL_HANDLE);
        return pipeline_rows_to_buffer_inverse_;
}

VkPipeline MulProgram::pipeline_rows_from_buffer(const bool inverse) const
{
        if (!inverse)
        {
                ASSERT(pipeline_rows_from_buffer_forward_ != VK_NULL_HANDLE);
                return pipeline_rows_from_buffer_forward_;
        }

        ASSERT(pipeline_rows_from_buffer_inverse_ != VK_NULL_HANDLE);
        return pipeline_rows_from_buffer_inverse_;
}

VkPipeline MulProgram::pipeline_columns_to_buffer(const bool inverse) const
{
        if (!inverse)
        {
                ASSERT(pipeline_columns_to_buffer_forward_ != VK_NULL_HANDLE);
                return pipeline_columns_to_buffer_forward_;
        }

        ASSERT(pipeline_columns_to_buffer_inverse_ != VK_NULL_HANDLE);
        return pipeline_columns_to_buffer_inverse_;
}

VkPipeline MulProgram::pipeline_columns_from_buffer(const bool inverse) const
{
        if (!inverse)
        {
                ASSERT(pipeline_columns_from_buffer_forward_ != VK_NULL_HANDLE);
                return pipeline_columns_from_buffer_forward_;
        }

        ASSERT(pipeline_columns_from_buffer_inverse_ != VK_NULL_HANDLE);
        return pipeline_columns_from_buffer_inverse_;
}

void MulProgram::create_pipelines(
        const std::int32_t n_1,
        const std::int32_t n_2,
        const std::int32_t m_1,
        const std::int32_t m_2,
        const std::uint32_t group_size_x,
        const std::uint32_t group_size_y)
{
        SpecializationConstants constants(n_1, n_2, m_1, m_2, group_size_x, group_size_y);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();

        constants.set_function(0, false);
        pipeline_rows_to_buffer_forward_ = create_compute_pipeline(info);
        constants.set_function(0, true);
        pipeline_rows_to_buffer_inverse_ = create_compute_pipeline(info);

        constants.set_function(1, false);
        pipeline_rows_from_buffer_forward_ = create_compute_pipeline(info);
        constants.set_function(1, true);
        pipeline_rows_from_buffer_inverse_ = create_compute_pipeline(info);

        constants.set_function(2, false);
        pipeline_columns_to_buffer_forward_ = create_compute_pipeline(info);
        constants.set_function(2, true);
        pipeline_columns_to_buffer_inverse_ = create_compute_pipeline(info);

        constants.set_function(3, false);
        pipeline_columns_from_buffer_forward_ = create_compute_pipeline(info);
        constants.set_function(3, true);
        pipeline_columns_from_buffer_inverse_ = create_compute_pipeline(info);
}

void MulProgram::delete_pipelines()
{
        pipeline_rows_to_buffer_forward_ = vulkan::handle::Pipeline();
        pipeline_rows_to_buffer_inverse_ = vulkan::handle::Pipeline();
        pipeline_rows_from_buffer_forward_ = vulkan::handle::Pipeline();
        pipeline_rows_from_buffer_inverse_ = vulkan::handle::Pipeline();
        pipeline_columns_to_buffer_forward_ = vulkan::handle::Pipeline();
        pipeline_columns_to_buffer_inverse_ = vulkan::handle::Pipeline();
        pipeline_columns_from_buffer_forward_ = vulkan::handle::Pipeline();
        pipeline_columns_from_buffer_inverse_ = vulkan::handle::Pipeline();
}
}
