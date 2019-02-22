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

#include "vulkan_shader.h"

#include <type_traits>

namespace gpgpu_convex_hull_compute_vulkan_implementation
{
std::vector<VkDescriptorSetLayoutBinding> PrepareMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 0;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

PrepareMemory::PrepareMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(vulkan::Descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())),
          m_descriptor_set(m_descriptors.create_descriptor_set())
{
}

VkDescriptorSetLayout PrepareMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& PrepareMemory::descriptor_set() const noexcept
{
        return m_descriptor_set;
}

void PrepareMemory::set_object_image(const vulkan::StorageImage& storage_image) const
{
        ASSERT(storage_image.format() == VK_FORMAT_R32_UINT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = storage_image.image_layout();
        image_info.imageView = storage_image.image_view();

        m_descriptors.update_descriptor_set(m_descriptor_set, 1, image_info);
}

void PrepareMemory::set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(m_descriptor_set, 0, buffer_info);
}

//

PrepareConstant::PrepareConstant()
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
                entry.offset = offsetof(Data, buffer_size);
                entry.size = sizeof(Data::buffer_size);
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

void PrepareConstant::set_line_size(uint32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.line_size), decltype(v)>);
        m_data.line_size = v;
}

void PrepareConstant::set_buffer_and_group_size(uint32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.buffer_size), decltype(v)>);
        m_data.buffer_size = v;
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(v)>);
        m_data.local_size_x = v;
}

const std::vector<VkSpecializationMapEntry>& PrepareConstant::entries() const noexcept
{
        return m_entries;
}

const void* PrepareConstant::data() const noexcept
{
        return &m_data;
}

size_t PrepareConstant::size() const noexcept
{
        return sizeof(m_data);
}

//

std::vector<VkDescriptorSetLayoutBinding> MergeMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 0;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

MergeMemory::MergeMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(vulkan::Descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())),
          m_descriptor_set(m_descriptors.create_descriptor_set())
{
}

VkDescriptorSetLayout MergeMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& MergeMemory::descriptor_set() const noexcept
{
        return m_descriptor_set;
}

void MergeMemory::set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(m_descriptor_set, 0, buffer_info);
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

void MergeConstant::set_local_size_x(uint32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(v)>);
        m_data.local_size_x = v;
}

const std::vector<VkSpecializationMapEntry>& MergeConstant::entries() const noexcept
{
        return m_entries;
}

const void* MergeConstant::data() const noexcept
{
        return &m_data;
}

size_t MergeConstant::size() const noexcept
{
        return sizeof(m_data);
}

//

std::vector<VkDescriptorSetLayoutBinding> FilterMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 0;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

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

FilterMemory::FilterMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(vulkan::Descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())),
          m_descriptor_set(m_descriptors.create_descriptor_set())
{
}

VkDescriptorSetLayout FilterMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& FilterMemory::descriptor_set() const noexcept
{
        return m_descriptor_set;
}

void FilterMemory::set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(m_descriptor_set, 0, buffer_info);
}

void FilterMemory::set_points(const vulkan::BufferWithHostVisibleMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(m_descriptor_set, 1, buffer_info);
}

void FilterMemory::set_point_count(const vulkan::BufferWithHostVisibleMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(m_descriptor_set, 2, buffer_info);
}

//

FilterConstant::FilterConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, line_size);
                entry.size = sizeof(Data::line_size);
                m_entries.push_back(entry);
        }
}

void FilterConstant::set_line_size(int32_t v)
{
        static_assert(std::is_same_v<decltype(m_data.line_size), decltype(v)>);
        m_data.line_size = v;
}

const std::vector<VkSpecializationMapEntry>& FilterConstant::entries() const noexcept
{
        return m_entries;
}

const void* FilterConstant::data() const noexcept
{
        return &m_data;
}

size_t FilterConstant::size() const noexcept
{
        return sizeof(m_data);
}
}
