/*
Copyright (C) 2017, 2018 Topological Manifold

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

#if defined(VULKAN_FOUND)

#include "descriptor.h"

namespace
{
vulkan::DescriptorSetLayout create_descriptor_set_layout(
        VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings)
{
        VkDescriptorSetLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = descriptor_set_layout_bindings.size();
        create_info.pBindings = descriptor_set_layout_bindings.data();

        return vulkan::DescriptorSetLayout(device, create_info);
}

std::vector<vulkan::UniformBufferWithHostVisibleMemory> create_uniform_buffers(
        const vulkan::Device& device, const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const std::vector<VkDeviceSize>& descriptor_set_layout_bindings_sizes)
{
        ASSERT(descriptor_set_layout_bindings.size() == descriptor_set_layout_bindings_sizes.size());

        std::vector<vulkan::UniformBufferWithHostVisibleMemory> uniform_buffers;

        for (size_t i = 0; i < descriptor_set_layout_bindings.size(); ++i)
        {
                ASSERT(descriptor_set_layout_bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

                uniform_buffers.emplace_back(device, descriptor_set_layout_bindings_sizes[i]);
        }

        return uniform_buffers;
}

vulkan::DescriptorPool create_descriptor_pool(VkDevice device,
                                              const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                                              uint32_t max_sets, VkDescriptorPoolCreateFlags flags)
{
        std::vector<VkDescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(descriptor_set_layout_bindings.size());

        for (const VkDescriptorSetLayoutBinding& binding : descriptor_set_layout_bindings)
        {
                VkDescriptorPoolSize pool_size = {};
                pool_size.type = binding.descriptorType;
                pool_size.descriptorCount = binding.descriptorCount;
                pool_sizes.push_back(pool_size);
        }

        VkDescriptorPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.poolSizeCount = pool_sizes.size();
        create_info.pPoolSizes = pool_sizes.data();
        create_info.maxSets = max_sets;
        create_info.flags = flags;

        return vulkan::DescriptorPool(device, create_info);
}

vulkan::DescriptorSet create_descriptor_set(
        VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const std::vector<vulkan::UniformBufferWithHostVisibleMemory>& descriptor_set_layout_uniform_buffers)
{
        ASSERT(descriptor_set_layout_bindings.size() == descriptor_set_layout_uniform_buffers.size());

        vulkan::DescriptorSet descriptor_set(device, descriptor_pool, descriptor_set_layout);

        const uint32_t size = descriptor_set_layout_bindings.size();

        std::vector<VkDescriptorBufferInfo> descriptor_buffer_info(size);
        std::vector<VkWriteDescriptorSet> write_descriptor_set(size);

        for (uint32_t i = 0; i < size; ++i)
        {
                descriptor_buffer_info[i] = {};
                descriptor_buffer_info[i].buffer = descriptor_set_layout_uniform_buffers[i];
                descriptor_buffer_info[i].offset = 0;
                descriptor_buffer_info[i].range = descriptor_set_layout_uniform_buffers[i].size();

                write_descriptor_set[i] = {};
                write_descriptor_set[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write_descriptor_set[i].dstSet = descriptor_set;
                write_descriptor_set[i].dstBinding = descriptor_set_layout_bindings[i].binding;
                write_descriptor_set[i].dstArrayElement = 0;

                write_descriptor_set[i].descriptorType = descriptor_set_layout_bindings[i].descriptorType;
                write_descriptor_set[i].descriptorCount = descriptor_set_layout_bindings[i].descriptorCount;

                ASSERT(descriptor_set_layout_bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                write_descriptor_set[i].pBufferInfo = &descriptor_buffer_info[i];
                // write_descriptor_set[i].pImageInfo = nullptr;
                // write_descriptor_set[i].pTexelBufferView = nullptr;
        }

        vkUpdateDescriptorSets(device, write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);

        return descriptor_set;
}
}

namespace vulkan
{
DescriptorWithBuffers::DescriptorWithBuffers(const vulkan::Device& device,
                                             const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                                             const std::vector<VkDeviceSize>& descriptor_set_layout_bindings_sizes)
        : m_descriptor_set_layout(create_descriptor_set_layout(device, descriptor_set_layout_bindings)),
          m_descriptor_set_layout_uniform_buffers(
                  create_uniform_buffers(device, descriptor_set_layout_bindings, descriptor_set_layout_bindings_sizes)),
          m_descriptor_pool(create_descriptor_pool(device, descriptor_set_layout_bindings, 1 /*max_sets*/,
                                                   VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)),
          m_descriptor_set(create_descriptor_set(device, m_descriptor_pool, m_descriptor_set_layout,
                                                 descriptor_set_layout_bindings, m_descriptor_set_layout_uniform_buffers))
{
}

VkDescriptorSetLayout DescriptorWithBuffers::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

VkDescriptorSet DescriptorWithBuffers::descriptor_set() const noexcept
{
        return m_descriptor_set;
}

void DescriptorWithBuffers::copy_to_buffer(uint32_t index, const Span<const void>& data) const
{
        ASSERT(index < m_descriptor_set_layout_uniform_buffers.size());
        ASSERT(data.size() == m_descriptor_set_layout_uniform_buffers[index].size());

        m_descriptor_set_layout_uniform_buffers[index].copy(data.data());
}
}

#endif
