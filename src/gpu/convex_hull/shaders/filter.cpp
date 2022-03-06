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

#include "filter.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::convex_hull
{
std::vector<VkDescriptorSetLayoutBinding> FilterMemory::descriptor_set_layout_bindings()
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
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINT_COUNT_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }

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

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, LINES_BINDING, buffer_info);
}

void FilterMemory::set_points(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, POINTS_BINDING, buffer_info);
}

void FilterMemory::set_point_count(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, POINT_COUNT_BINDING, buffer_info);
}

//

FilterConstant::FilterConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, line_size);
                entry.size = sizeof(Data::line_size);
                entries_.push_back(entry);
        }
}

void FilterConstant::set_line_size(const std::int32_t v)
{
        static_assert(std::is_same_v<decltype(data_.line_size), std::remove_const_t<decltype(v)>>);
        data_.line_size = v;
}

const std::vector<VkSpecializationMapEntry>& FilterConstant::entries() const
{
        return entries_;
}

const void* FilterConstant::data() const
{
        return &data_;
}

std::size_t FilterConstant::size() const
{
        return sizeof(data_);
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
        constant_.set_line_size(height);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constant_;
        pipeline_ = create_compute_pipeline(info);
}

void FilterProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
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
