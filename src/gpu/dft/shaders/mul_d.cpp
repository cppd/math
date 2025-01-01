/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "mul_d.h"

#include <src/com/error.h>
#include <src/gpu/dft/code/code.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/compute.h>

#include <vulkan/vulkan_core.h>

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
                std::uint32_t group_size_x;
                std::uint32_t group_size_y;
                std::int32_t rows;
                std::int32_t columns;
        };

        static constexpr std::array<VkSpecializationMapEntry, 4> ENTRIES{
                {{0, offsetof(Data, group_size_x), sizeof(Data::group_size_x)},
                 {1, offsetof(Data, group_size_y), sizeof(Data::group_size_y)},
                 {2, offsetof(Data, rows), sizeof(Data::rows)},
                 {3, offsetof(Data, columns), sizeof(Data::columns)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(const std::uint32_t group_size_x, const std::uint32_t group_size_y)
                : data_{.group_size_x = group_size_x, .group_size_y = group_size_y, .rows = 0, .columns = 0}
        {
        }

        SpecializationConstants(const SpecializationConstants&) = delete;
        SpecializationConstants& operator=(const SpecializationConstants&) = delete;

        void set_rows_columns(const std::int32_t rows, const std::int32_t columns)
        {
                data_.rows = rows;
                data_.columns = columns;
        }

        [[nodiscard]] const VkSpecializationInfo& info() const
        {
                return info_;
        }
};
}

std::vector<VkDescriptorSetLayoutBinding> MulDMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = DIAGONAL_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = DATA_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

MulDMemory::MulDMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned MulDMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& MulDMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void MulDMemory::set(const vulkan::Buffer& diagonal, const vulkan::Buffer& data) const
{
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        ASSERT(diagonal.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        infos.emplace_back(
                DESCRIPTOR_INDEX, DIAGONAL_BINDING,
                VkDescriptorBufferInfo{.buffer = diagonal.handle(), .offset = 0, .range = diagonal.size()});

        ASSERT(data.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        infos.emplace_back(
                DESCRIPTOR_INDEX, DATA_BINDING,
                VkDescriptorBufferInfo{.buffer = data.handle(), .offset = 0, .range = data.size()});

        descriptors_.update_descriptor_set(infos);
}

//

MulDProgram::MulDProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, MulDMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {MulDMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_mul_d_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

VkDescriptorSetLayout MulDProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout MulDProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline MulDProgram::pipeline_rows() const
{
        ASSERT(pipeline_rows_ != VK_NULL_HANDLE);
        return pipeline_rows_;
}

VkPipeline MulDProgram::pipeline_columns() const
{
        ASSERT(pipeline_columns_ != VK_NULL_HANDLE);
        return pipeline_columns_;
}

void MulDProgram::create_pipelines(
        const std::uint32_t n_1,
        const std::uint32_t n_2,
        const std::uint32_t m_1,
        const std::uint32_t m_2,
        const std::uint32_t group_size_x,
        const std::uint32_t group_size_y)
{
        SpecializationConstants constants(group_size_x, group_size_y);

        vulkan::pipeline::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();

        constants.set_rows_columns(n_2, m_1);
        pipeline_rows_ = vulkan::pipeline::create_compute_pipeline(info);

        constants.set_rows_columns(n_1, m_2);
        pipeline_columns_ = vulkan::pipeline::create_compute_pipeline(info);
}

void MulDProgram::delete_pipelines()
{
        pipeline_rows_ = {};
        pipeline_columns_ = {};
}
}
