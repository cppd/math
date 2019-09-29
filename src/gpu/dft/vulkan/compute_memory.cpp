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

#include "compute_memory.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> DftCopyInputMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = INPUT_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OUTPUT_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftCopyInputMemory::DftCopyInputMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftCopyInputMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout DftCopyInputMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& DftCopyInputMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftCopyInputMemory::set_input(VkSampler sampler, const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.usage() & VK_IMAGE_USAGE_SAMPLED_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image.image_view();
        image_info.sampler = sampler;

        m_descriptors.update_descriptor_set(0, INPUT_BINDING, image_info);
}

void DftCopyInputMemory::set_output(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, OUTPUT_BINDING, buffer_info);
}

//

DftCopyInputConstant::DftCopyInputConstant()
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
}

void DftCopyInputConstant::set_group_size(uint32_t x, uint32_t y)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(x)>);
        m_data.local_size_x = x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(y)>);
        m_data.local_size_y = y;
}

const std::vector<VkSpecializationMapEntry>& DftCopyInputConstant::entries() const
{
        return m_entries;
}

const void* DftCopyInputConstant::data() const
{
        return &m_data;
}

size_t DftCopyInputConstant::size() const
{
        return sizeof(m_data);
}
}
