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

namespace gpgpu_vulkan
{
namespace convex_hull_compute_implementation
{
std::vector<VkDescriptorSetLayoutBinding> ShaderMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 0;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 2;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

ShaderMemory::ShaderMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(vulkan::Descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings()))
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        m_descriptor_set = m_descriptors.create_and_update_descriptor_set(bindings, infos);
}

VkDescriptorSetLayout ShaderMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

VkDescriptorSet ShaderMemory::descriptor_set() const noexcept
{
        return m_descriptor_set;
}

void ShaderMemory::set_object_image(const vulkan::StorageImage& storage_image) const
{
        ASSERT(storage_image.format() == VK_FORMAT_R32_UINT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = storage_image.image_layout();
        image_info.imageView = storage_image.image_view();

        m_descriptors.update_descriptor_set(m_descriptor_set, 0, image_info);
}

void ShaderMemory::set_points(const vulkan::BufferWithHostVisibleMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(m_descriptor_set, 1, buffer_info);
}

void ShaderMemory::set_point_count(const vulkan::BufferWithHostVisibleMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(m_descriptor_set, 2, buffer_info);
}

//

ShaderConstant::ShaderConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, local_size_x);
                entry.size = sizeof(Data::local_size_x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, local_size_y);
                entry.size = sizeof(Data::local_size_y);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, local_size_z);
                entry.size = sizeof(Data::local_size_z);
                m_entries.push_back(entry);
        }
}

void ShaderConstant::set_local_size_x(uint32_t x)
{
        m_data.local_size_x = x;
}
void ShaderConstant::set_local_size_y(uint32_t y)
{
        m_data.local_size_y = y;
}
void ShaderConstant::set_local_size_z(uint32_t z)
{
        m_data.local_size_z = z;
}

const std::vector<VkSpecializationMapEntry>* ShaderConstant::entries() const noexcept
{
        return &m_entries;
}

const void* ShaderConstant::data() const noexcept
{
        return &m_data;
}

size_t ShaderConstant::size() const noexcept
{
        return sizeof(m_data);
}
}
}
