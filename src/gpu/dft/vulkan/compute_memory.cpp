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
                b.binding = SRC_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DST_BINDING;
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

        m_descriptors.update_descriptor_set(0, SRC_BINDING, image_info);
}

void DftCopyInputMemory::set_output(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, DST_BINDING, buffer_info);
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
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, x);
                entry.size = sizeof(Data::x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, y);
                entry.size = sizeof(Data::y);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, width);
                entry.size = sizeof(Data::width);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, height);
                entry.size = sizeof(Data::height);
                m_entries.push_back(entry);
        }
}

void DftCopyInputConstant::set(int32_t local_size_x, int32_t local_size_y, int32_t x, int32_t y, int32_t width, int32_t height)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(local_size_x)>);
        m_data.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(local_size_y)>);
        m_data.local_size_y = local_size_y;
        static_assert(std::is_same_v<decltype(m_data.x), decltype(x)>);
        m_data.x = x;
        static_assert(std::is_same_v<decltype(m_data.y), decltype(y)>);
        m_data.y = y;
        static_assert(std::is_same_v<decltype(m_data.width), decltype(width)>);
        m_data.width = width;
        static_assert(std::is_same_v<decltype(m_data.height), decltype(height)>);
        m_data.height = height;
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

//

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

//

std::vector<VkDescriptorSetLayoutBinding> DftBitReverseMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftBitReverseMemory::DftBitReverseMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftBitReverseMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout DftBitReverseMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& DftBitReverseMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftBitReverseMemory::set_buffer(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

DftBitReverseConstant::DftBitReverseConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, group_size);
                entry.size = sizeof(Data::group_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, data_size);
                entry.size = sizeof(Data::data_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, n_mask);
                entry.size = sizeof(Data::n_mask);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, n_bits);
                entry.size = sizeof(Data::n_bits);
                m_entries.push_back(entry);
        }
}

void DftBitReverseConstant::set(uint32_t group_size, uint32_t data_size, uint32_t n_mask, uint32_t n_bits)
{
        static_assert(std::is_same_v<decltype(m_data.group_size), decltype(group_size)>);
        m_data.group_size = group_size;
        static_assert(std::is_same_v<decltype(m_data.data_size), decltype(data_size)>);
        m_data.data_size = data_size;
        static_assert(std::is_same_v<decltype(m_data.n_mask), decltype(n_mask)>);
        m_data.n_mask = n_mask;
        static_assert(std::is_same_v<decltype(m_data.n_bits), decltype(n_bits)>);
        m_data.n_bits = n_bits;
}

const std::vector<VkSpecializationMapEntry>& DftBitReverseConstant::entries() const
{
        return m_entries;
}

const void* DftBitReverseConstant::data() const
{
        return &m_data;
}

size_t DftBitReverseConstant::size() const
{
        return sizeof(m_data);
}

//

std::vector<VkDescriptorSetLayoutBinding> DftFftGlobalMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftFftGlobalMemory::DftFftGlobalMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(vulkan::BufferMemoryType::HostVisible, device, family_indices,
                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Data));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);

                bindings.push_back(DATA_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned DftFftGlobalMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout DftFftGlobalMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& DftFftGlobalMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftFftGlobalMemory::set_data(int data_size, float two_pi_div_m, int n_div_2_mask, int m_div_2) const
{
        Data d;
        d.data_size = data_size;
        d.two_pi_div_m = two_pi_div_m;
        d.n_div_2_mask = n_div_2_mask;
        d.m_div_2 = m_div_2;
        vulkan::map_and_write_to_buffer(m_uniform_buffers[0], d);
}

void DftFftGlobalMemory::set_buffer(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

DftFftGlobalConstant::DftFftGlobalConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, group_size);
                entry.size = sizeof(Data::group_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, inverse);
                entry.size = sizeof(Data::inverse);
                m_entries.push_back(entry);
        }
}

void DftFftGlobalConstant::set(uint32_t group_size, bool inverse)
{
        static_assert(std::is_same_v<decltype(m_data.group_size), decltype(group_size)>);
        m_data.group_size = group_size;
        static_assert(std::is_same_v<decltype(m_data.inverse), uint32_t>);
        m_data.inverse = inverse ? 1 : 0;
}

const std::vector<VkSpecializationMapEntry>& DftFftGlobalConstant::entries() const
{
        return m_entries;
}

const void* DftFftGlobalConstant::data() const
{
        return &m_data;
}

size_t DftFftGlobalConstant::size() const
{
        return sizeof(m_data);
}

//

std::vector<VkDescriptorSetLayoutBinding> DftFftSharedMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftFftSharedMemory::DftFftSharedMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftFftSharedMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout DftFftSharedMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& DftFftSharedMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftFftSharedMemory::set_buffer(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

DftFftSharedConstant::DftFftSharedConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, inverse);
                entry.size = sizeof(Data::inverse);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, data_size);
                entry.size = sizeof(Data::data_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, n);
                entry.size = sizeof(Data::n);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, n_mask);
                entry.size = sizeof(Data::n_mask);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, n_bits);
                entry.size = sizeof(Data::n_bits);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, shared_size);
                entry.size = sizeof(Data::shared_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 6;
                entry.offset = offsetof(Data, reverse_input);
                entry.size = sizeof(Data::reverse_input);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 7;
                entry.offset = offsetof(Data, group_size);
                entry.size = sizeof(Data::group_size);
                m_entries.push_back(entry);
        }
}

void DftFftSharedConstant::set(bool inverse, uint32_t data_size, uint32_t n, uint32_t n_mask, uint32_t n_bits,
                               uint32_t shared_size, bool reverse_input, uint32_t group_size)
{
        static_assert(std::is_same_v<decltype(m_data.inverse), uint32_t>);
        m_data.inverse = inverse ? 1 : 0;
        static_assert(std::is_same_v<decltype(m_data.data_size), decltype(data_size)>);
        m_data.data_size = data_size;
        static_assert(std::is_same_v<decltype(m_data.n), decltype(n)>);
        m_data.n = n;
        static_assert(std::is_same_v<decltype(m_data.n_mask), decltype(n_mask)>);
        m_data.n_mask = n_mask;
        static_assert(std::is_same_v<decltype(m_data.n_bits), decltype(n_bits)>);
        m_data.n_bits = n_bits;
        static_assert(std::is_same_v<decltype(m_data.shared_size), decltype(shared_size)>);
        m_data.shared_size = shared_size;
        static_assert(std::is_same_v<decltype(m_data.reverse_input), uint32_t>);
        m_data.reverse_input = reverse_input ? 1 : 0;
        static_assert(std::is_same_v<decltype(m_data.group_size), decltype(group_size)>);
        m_data.group_size = group_size;
}

const std::vector<VkSpecializationMapEntry>& DftFftSharedConstant::entries() const
{
        return m_entries;
}

const void* DftFftSharedConstant::data() const
{
        return &m_data;
}

size_t DftFftSharedConstant::size() const
{
        return sizeof(m_data);
}

//

std::vector<VkDescriptorSetLayoutBinding> DftMulMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftMulMemory::DftMulMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftMulMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout DftMulMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& DftMulMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftMulMemory::set_data(const vulkan::BufferWithMemory& data) const
{
        ASSERT(data.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = data;
        buffer_info.offset = 0;
        buffer_info.range = data.size();

        m_descriptors.update_descriptor_set(0, DATA_BINDING, buffer_info);
}

void DftMulMemory::set_buffer(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

DftMulConstant::DftMulConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, function_index);
                entry.size = sizeof(Data::function_index);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, n1);
                entry.size = sizeof(Data::n1);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, n2);
                entry.size = sizeof(Data::n2);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, m1);
                entry.size = sizeof(Data::m1);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, m2);
                entry.size = sizeof(Data::m2);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, inverse);
                entry.size = sizeof(Data::inverse);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 6;
                entry.offset = offsetof(Data, group_size_x);
                entry.size = sizeof(Data::group_size_x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 7;
                entry.offset = offsetof(Data, group_size_y);
                entry.size = sizeof(Data::group_size_y);
                m_entries.push_back(entry);
        }
}

void DftMulConstant::set(int32_t function_index, int32_t n1, int32_t n2, int32_t m1, int32_t m2, bool inverse,
                         uint32_t group_size_x, uint32_t group_size_y)
{
        static_assert(std::is_same_v<decltype(m_data.function_index), int32_t>);
        m_data.function_index = function_index;
        static_assert(std::is_same_v<decltype(m_data.n1), decltype(n1)>);
        m_data.n1 = n1;
        static_assert(std::is_same_v<decltype(m_data.n2), decltype(n2)>);
        m_data.n2 = n2;
        static_assert(std::is_same_v<decltype(m_data.m1), decltype(m1)>);
        m_data.m1 = m1;
        static_assert(std::is_same_v<decltype(m_data.m2), decltype(m2)>);
        m_data.m2 = m2;
        static_assert(std::is_same_v<decltype(m_data.inverse), uint32_t>);
        m_data.inverse = inverse ? 1 : 0;
        static_assert(std::is_same_v<decltype(m_data.group_size_x), decltype(group_size_x)>);
        m_data.group_size_x = group_size_x;
        static_assert(std::is_same_v<decltype(m_data.group_size_y), decltype(group_size_y)>);
        m_data.group_size_y = group_size_y;
}

const std::vector<VkSpecializationMapEntry>& DftMulConstant::entries() const
{
        return m_entries;
}

const void* DftMulConstant::data() const
{
        return &m_data;
}

size_t DftMulConstant::size() const
{
        return sizeof(m_data);
}

//

std::vector<VkDescriptorSetLayoutBinding> DftMulDMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DIAGONAL_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

DftMulDMemory::DftMulDMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned DftMulDMemory::set_number()
{
        return SET_NUMBER;
}

VkDescriptorSetLayout DftMulDMemory::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& DftMulDMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void DftMulDMemory::set_diagonal(const vulkan::BufferWithMemory& diagonal) const
{
        ASSERT(diagonal.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = diagonal;
        buffer_info.offset = 0;
        buffer_info.range = diagonal.size();

        m_descriptors.update_descriptor_set(0, DIAGONAL_BINDING, buffer_info);
}

void DftMulDMemory::set_data(const vulkan::BufferWithMemory& data) const
{
        ASSERT(data.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = data;
        buffer_info.offset = 0;
        buffer_info.range = data.size();

        m_descriptors.update_descriptor_set(0, DATA_BINDING, buffer_info);
}

//

DftMulDConstant::DftMulDConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, group_size_x);
                entry.size = sizeof(Data::group_size_x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, group_size_y);
                entry.size = sizeof(Data::group_size_y);
                m_entries.push_back(entry);
        }

        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, rows);
                entry.size = sizeof(Data::rows);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, columns);
                entry.size = sizeof(Data::columns);
                m_entries.push_back(entry);
        }
}

void DftMulDConstant::set(uint32_t group_size_x, uint32_t group_size_y, int32_t rows, int32_t columns)
{
        static_assert(std::is_same_v<decltype(m_data.group_size_x), decltype(group_size_x)>);
        m_data.group_size_x = group_size_x;
        static_assert(std::is_same_v<decltype(m_data.group_size_y), decltype(group_size_y)>);
        m_data.group_size_y = group_size_y;
        static_assert(std::is_same_v<decltype(m_data.rows), decltype(rows)>);
        m_data.rows = rows;
        static_assert(std::is_same_v<decltype(m_data.columns), decltype(columns)>);
        m_data.columns = columns;
}

const std::vector<VkSpecializationMapEntry>& DftMulDConstant::entries() const
{
        return m_entries;
}

const void* DftMulDConstant::data() const
{
        return &m_data;
}

size_t DftMulDConstant::size() const
{
        return sizeof(m_data);
}
}
