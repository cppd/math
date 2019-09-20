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
std::vector<VkDescriptorSetLayoutBinding> PencilSketchComputeMemory::descriptor_set_layout_bindings()
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
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OBJECTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

PencilSketchComputeMemory::PencilSketchComputeMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned PencilSketchComputeMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout PencilSketchComputeMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& PencilSketchComputeMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void PencilSketchComputeMemory::set_input(VkSampler sampler, const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.usage() & VK_IMAGE_USAGE_SAMPLED_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image.image_view();
        image_info.sampler = sampler;

        m_descriptors.update_descriptor_set(0, INPUT_BINDING, image_info);
}

void PencilSketchComputeMemory::set_output_image(const vulkan::ImageWithMemory& storage_image) const
{
        ASSERT(storage_image.format() == VK_FORMAT_R32G32B32A32_SFLOAT);
        ASSERT(storage_image.usage() & VK_IMAGE_USAGE_STORAGE_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = storage_image.image_view();

        m_descriptors.update_descriptor_set(0, OUTPUT_BINDING, image_info);
}

void PencilSketchComputeMemory::set_object_image(const vulkan::ImageWithMemory& storage_image) const
{
        ASSERT(storage_image.format() == VK_FORMAT_R32_UINT);
        ASSERT(storage_image.usage() & VK_IMAGE_USAGE_STORAGE_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = storage_image.image_view();

        m_descriptors.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

//

PencilSketchComputeConstant::PencilSketchComputeConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, local_size);
                entry.size = sizeof(Data::local_size);
                m_entries.push_back(entry);
        }
}

void PencilSketchComputeConstant::set_group_size(uint32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.local_size), decltype(v)>);
        m_data.local_size = v;
}

const std::vector<VkSpecializationMapEntry>& PencilSketchComputeConstant::entries() const
{
        return m_entries;
}

const void* PencilSketchComputeConstant::data() const
{
        return &m_data;
}

size_t PencilSketchComputeConstant::size() const
{
        return sizeof(m_data);
}

//

std::vector<VkDescriptorSetLayoutBinding> PencilSketchLuminanceMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = IMAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

PencilSketchLuminanceMemory::PencilSketchLuminanceMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned PencilSketchLuminanceMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout PencilSketchLuminanceMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& PencilSketchLuminanceMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void PencilSketchLuminanceMemory::set_image(const vulkan::ImageWithMemory& storage_image) const
{
        ASSERT(storage_image.format() == VK_FORMAT_R32G32B32A32_SFLOAT);
        ASSERT(storage_image.usage() & VK_IMAGE_USAGE_STORAGE_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = storage_image.image_view();

        m_descriptors.update_descriptor_set(0, IMAGE_BINDING, image_info);
}

//

PencilSketchLuminanceConstant::PencilSketchLuminanceConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, local_size);
                entry.size = sizeof(Data::local_size);
                m_entries.push_back(entry);
        }
}

void PencilSketchLuminanceConstant::set_group_size(uint32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.local_size), decltype(v)>);
        m_data.local_size = v;
}

const std::vector<VkSpecializationMapEntry>& PencilSketchLuminanceConstant::entries() const
{
        return m_entries;
}

const void* PencilSketchLuminanceConstant::data() const
{
        return &m_data;
}

size_t PencilSketchLuminanceConstant::size() const
{
        return sizeof(m_data);
}
}
