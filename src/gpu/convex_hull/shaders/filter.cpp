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

#include "filter.h"

#include <src/com/error.h>
#include <src/gpu/convex_hull/code/code.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/compute.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::convex_hull
{
namespace
{
class SpecializationConstants final
{
        struct Data final
        {
                std::int32_t line_size;
        };

        static constexpr std::array<VkSpecializationMapEntry, 1> ENTRIES{
                {{0, offsetof(Data, line_size), sizeof(Data::line_size)}}};

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        explicit SpecializationConstants(const std::int32_t line_size)
                : data_{.line_size = line_size}
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

std::vector<VkDescriptorSetLayoutBinding> FilterMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(3);

        bindings.push_back(
                {.binding = LINES_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = POINTS_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = POINT_COUNT_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

FilterMemory::FilterMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned FilterMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& FilterMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void FilterMemory::set_lines(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        descriptors_.update_descriptor_set(
                0, LINES_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
}

void FilterMemory::set_points(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        descriptors_.update_descriptor_set(
                0, POINTS_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
}

void FilterMemory::set_point_count(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        descriptors_.update_descriptor_set(
                0, POINT_COUNT_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
}

//

FilterProgram::FilterProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, FilterMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {FilterMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_filter_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

void FilterProgram::create_pipeline(const unsigned height)
{
        const SpecializationConstants constants(height);

        vulkan::pipeline::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();
        pipeline_ = vulkan::pipeline::create_compute_pipeline(info);
}

void FilterProgram::delete_pipeline()
{
        pipeline_ = {};
}

VkDescriptorSetLayout FilterProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout FilterProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline FilterProgram::pipeline() const
{
        return pipeline_;
}
}
