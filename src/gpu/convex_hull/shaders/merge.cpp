/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

#include <type_traits>

namespace gpu::convex_hull
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

MergeMemory::MergeMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned MergeMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& MergeMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void MergeMemory::set_lines(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, LINES_BINDING, buffer_info);
}

//

MergeConstant::MergeConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, line_size);
                entry.size = sizeof(Data::line_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, iteration_count);
                entry.size = sizeof(Data::iteration_count);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, local_size_x);
                entry.size = sizeof(Data::local_size_x);
                m_entries.push_back(entry);
        }
}

void MergeConstant::set_line_size(int32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.line_size), decltype(v)>);
        m_data.line_size = v;
}

void MergeConstant::set_iteration_count(int32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.iteration_count), decltype(v)>);
        m_data.iteration_count = v;
}

void MergeConstant::set_local_size_x(int32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(v)>);
        m_data.local_size_x = v;
}

const std::vector<VkSpecializationMapEntry>& MergeConstant::entries() const
{
        return m_entries;
}

const void* MergeConstant::data() const
{
        return &m_data;
}

size_t MergeConstant::size() const
{
        return sizeof(m_data);
}

//

MergeProgram::MergeProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, MergeMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {MergeMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, code_merge_comp(), "main")
{
}

void MergeProgram::create_pipeline(unsigned height, unsigned local_size_x, unsigned iteration_count)
{
        m_constant.set_line_size(height);
        m_constant.set_local_size_x(local_size_x);
        m_constant.set_iteration_count(iteration_count);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void MergeProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}

VkDescriptorSetLayout MergeProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout MergeProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline MergeProgram::pipeline() const
{
        return m_pipeline;
}
}
