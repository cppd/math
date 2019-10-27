/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "compute_filter.h"

#include "shader_source.h"

#include "gpu/convex_hull/com/com.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

#include <type_traits>

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> ConvexHullFilterMemory::descriptor_set_layout_bindings()
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

ConvexHullFilterMemory::ConvexHullFilterMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned ConvexHullFilterMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout ConvexHullFilterMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& ConvexHullFilterMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void ConvexHullFilterMemory::set_lines(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, LINES_BINDING, buffer_info);
}

void ConvexHullFilterMemory::set_points(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, POINTS_BINDING, buffer_info);
}

void ConvexHullFilterMemory::set_point_count(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, POINT_COUNT_BINDING, buffer_info);
}

//

ConvexHullFilterConstant::ConvexHullFilterConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, line_size);
                entry.size = sizeof(Data::line_size);
                m_entries.push_back(entry);
        }
}

void ConvexHullFilterConstant::set_line_size(int32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.line_size), decltype(v)>);
        m_data.line_size = v;
}

const std::vector<VkSpecializationMapEntry>& ConvexHullFilterConstant::entries() const
{
        return m_entries;
}

const void* ConvexHullFilterConstant::data() const
{
        return &m_data;
}

size_t ConvexHullFilterConstant::size() const
{
        return sizeof(m_data);
}

//

ConvexHullProgramFilter::ConvexHullProgramFilter(const vulkan::VulkanInstance& instance)
        : m_instance(instance),
          m_memory(instance.device()),
          m_shader(instance.device(), convex_hull_filter_comp(), "main"),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(instance.device(), {m_memory.set_number()}, {m_memory.descriptor_set_layout()}))
{
}

void ConvexHullProgramFilter::create_buffers(unsigned height, const vulkan::BufferWithMemory& lines_buffer,
                                             const vulkan::BufferWithMemory& points_buffer,
                                             const vulkan::BufferWithMemory& point_count_buffer)
{
        m_memory.set_lines(lines_buffer);
        m_memory.set_points(points_buffer);
        m_memory.set_point_count(point_count_buffer);

        m_constant.set_line_size(height);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_instance.device();
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void ConvexHullProgramFilter::delete_buffers()
{
        m_pipeline = vulkan::Pipeline();
}

void ConvexHullProgramFilter::commands(VkCommandBuffer command_buffer) const
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, m_memory.set_number(),
                                1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);
        vkCmdDispatch(command_buffer, 1, 1, 1);
}
}
