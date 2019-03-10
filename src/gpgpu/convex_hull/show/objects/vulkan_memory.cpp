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

#include "vulkan_memory.h"

namespace gpgpu_convex_hull_show_vulkan_implementation
{
std::vector<VkDescriptorSetLayoutBinding> ShaderMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

ShaderMemory::ShaderMemory(const vulkan::Device& device, const std::vector<uint32_t>& family_indices)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Data));
                m_data_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);

                bindings.push_back(DATA_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned ShaderMemory::set_number() noexcept
{
        return SET_NUMBER;
}

VkDescriptorSetLayout ShaderMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& ShaderMemory::descriptor_set() const noexcept
{
        return m_descriptors.descriptor_set(0);
}

void ShaderMemory::set_matrix(const mat4& matrix) const
{
        decltype(Data().matrix) m = transpose(to_matrix<float>(matrix));
        m_uniform_buffers[m_data_buffer_index].write(offsetof(Data, matrix), m);
}

void ShaderMemory::set_brightness(float brightness) const
{
        decltype(Data().brightness) b = brightness;
        m_uniform_buffers[m_data_buffer_index].write(offsetof(Data, brightness), b);
}

void ShaderMemory::set_points(const vulkan::BufferWithHostVisibleMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, POINTS_BINDING, buffer_info);
}
}
