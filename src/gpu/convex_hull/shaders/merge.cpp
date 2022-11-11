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

#include "merge.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline_compute.h>

namespace ns::gpu::convex_hull
{
std::vector<VkDescriptorSetLayoutBinding> MergeMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

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

MergeMemory::MergeMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned MergeMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& MergeMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void MergeMemory::set_lines(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, LINES_BINDING, buffer_info);
}

//

MergeConstant::MergeConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, line_size);
                entry.size = sizeof(Data::line_size);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, iteration_count);
                entry.size = sizeof(Data::iteration_count);
                entries_.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, local_size_x);
                entry.size = sizeof(Data::local_size_x);
                entries_.push_back(entry);
        }
}

void MergeConstant::set_line_size(const std::int32_t v)
{
        static_assert(std::is_same_v<decltype(data_.line_size), std::remove_const_t<decltype(v)>>);
        data_.line_size = v;
}

void MergeConstant::set_iteration_count(const std::int32_t v)
{
        static_assert(std::is_same_v<decltype(data_.iteration_count), std::remove_const_t<decltype(v)>>);
        data_.iteration_count = v;
}

void MergeConstant::set_local_size_x(const std::int32_t v)
{
        static_assert(std::is_same_v<decltype(data_.local_size_x), std::remove_const_t<decltype(v)>>);
        data_.local_size_x = v;
}

VkSpecializationInfo MergeConstant::info() const
{
        VkSpecializationInfo info = {};
        info.mapEntryCount = entries_.size();
        info.pMapEntries = entries_.data();
        info.dataSize = sizeof(data_);
        info.pData = &data_;
        return info;
}

//

MergeProgram::MergeProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, MergeMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {MergeMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_merge_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

void MergeProgram::create_pipeline(const unsigned height, const unsigned local_size_x, const unsigned iteration_count)
{
        const VkSpecializationInfo constant_info = constant_.info();

        constant_.set_line_size(height);
        constant_.set_local_size_x(local_size_x);
        constant_.set_iteration_count(iteration_count);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_info;
        pipeline_ = create_compute_pipeline(info);
}

void MergeProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}

VkDescriptorSetLayout MergeProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout MergeProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline MergeProgram::pipeline() const
{
        return pipeline_;
}
}
