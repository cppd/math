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

#include "compute_copy_output.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> DftCopyOutputMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SRC_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DST_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftCopyOutputMemory::DftCopyOutputMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftCopyOutputMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout DftCopyOutputMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& DftCopyOutputMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftCopyOutputMemory::set_input(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, SRC_BINDING, buffer_info);
}

void DftCopyOutputMemory::set_output(const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.image_view();

        m_descriptors.update_descriptor_set(0, DST_BINDING, image_info);
}

//

DftCopyOutputConstant::DftCopyOutputConstant()
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
                entry.offset = offsetof(Data, to_mul);
                entry.size = sizeof(Data::to_mul);
                m_entries.push_back(entry);
        }
}

void DftCopyOutputConstant::set_group_size(uint32_t x, uint32_t y)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(x)>);
        m_data.local_size_x = x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(y)>);
        m_data.local_size_y = y;
}

void DftCopyOutputConstant::set_to_mul(float v)
{
        static_assert(std::is_same_v<decltype(m_data.to_mul), decltype(v)>);
        m_data.to_mul = v;
}

const std::vector<VkSpecializationMapEntry>& DftCopyOutputConstant::entries() const
{
        return m_entries;
}

const void* DftCopyOutputConstant::data() const
{
        return &m_data;
}

size_t DftCopyOutputConstant::size() const
{
        return sizeof(m_data);
}
}
