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

#include "descriptor.h"

#include "com/error.h"
#include "com/print.h"

namespace
{
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
                pool_size.descriptorCount = max_sets * binding.descriptorCount;
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

VkWriteDescriptorSet create_write_descriptor_set(VkDescriptorSet descriptor_set,
                                                 const VkDescriptorSetLayoutBinding& descriptor_set_layout_binding,
                                                 const Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>& descriptor_info)
{
        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        write_descriptor_set.dstSet = descriptor_set;
        write_descriptor_set.dstBinding = descriptor_set_layout_binding.binding;
        write_descriptor_set.dstArrayElement = 0;

        write_descriptor_set.descriptorType = descriptor_set_layout_binding.descriptorType;
        write_descriptor_set.descriptorCount = descriptor_set_layout_binding.descriptorCount;

        auto buffer = [&](const VkDescriptorBufferInfo& info) noexcept
        {
                ASSERT(descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                       descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
                write_descriptor_set.pBufferInfo = &info;
        };
        auto image = [&](const VkDescriptorImageInfo& info) noexcept
        {
                ASSERT(descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                       descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
                write_descriptor_set.pImageInfo = &info;
        };
        visit(Visitors{buffer, image}, descriptor_info);

        return write_descriptor_set;

        // write_descriptor_set.pTexelBufferView = nullptr;
}

std::unordered_map<uint32_t, uint32_t> create_binding_map(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
        std::unordered_map<uint32_t, uint32_t> map;
        for (size_t index = 0; index < bindings.size(); ++index)
        {
                if (!map.emplace(bindings[index].binding, index).second)
                {
                        error("Multiple binding " + to_string(bindings[index].binding) + " in descriptor set layout bindings");
                }
        }
        return map;
}
}

namespace vulkan
{
DescriptorSetLayout create_descriptor_set_layout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
        VkDescriptorSetLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = bindings.size();
        create_info.pBindings = bindings.data();

        return DescriptorSetLayout(device, create_info);
}

Descriptors::Descriptors(VkDevice device, uint32_t max_sets, VkDescriptorSetLayout descriptor_set_layout,
                         const std::vector<VkDescriptorSetLayoutBinding>& bindings)
        : m_device(device),
          m_descriptor_set_layout(descriptor_set_layout),
          m_descriptor_pool(
                  create_descriptor_pool(device, bindings, max_sets, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)),
          m_descriptor_set_layout_bindings(bindings),
          m_binding_map(create_binding_map(m_descriptor_set_layout_bindings))
{
        ASSERT(descriptor_set_layout != VK_NULL_HANDLE);
}

const VkDescriptorSetLayoutBinding& Descriptors::find_layout_binding(uint32_t binding) const
{
        auto i = m_binding_map.find(binding);
        if (i == m_binding_map.cend())
        {
                error("No binding " + to_string(binding) + " in the descriptor set layout bindings");
        }

        uint32_t index = i->second;

        ASSERT(index < m_descriptor_set_layout_bindings.size());

        return m_descriptor_set_layout_bindings[index];
}

DescriptorSet Descriptors::create_and_update_descriptor_set(
        const std::vector<uint32_t>& bindings,
        const std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>>& descriptor_infos) const
{
        DescriptorSet descriptor_set(m_device, m_descriptor_pool, m_descriptor_set_layout);

        update_descriptor_set(descriptor_set, bindings, descriptor_infos);

        return descriptor_set;
}

DescriptorSet Descriptors::create_descriptor_set() const
{
        return DescriptorSet(m_device, m_descriptor_pool, m_descriptor_set_layout);
}

void Descriptors::update_descriptor_set(VkDescriptorSet descriptor_set, uint32_t binding,
                                        const Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>& info) const
{
        VkWriteDescriptorSet write = create_write_descriptor_set(descriptor_set, find_layout_binding(binding), info);

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
}

void Descriptors::update_descriptor_set(
        VkDescriptorSet descriptor_set, const std::vector<uint32_t>& bindings,
        const std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>>& descriptor_infos) const
{
        ASSERT(bindings.size() == descriptor_infos.size());

        std::vector<VkWriteDescriptorSet> write(bindings.size());

        for (size_t i = 0; i < bindings.size(); ++i)
        {
                write[i] = create_write_descriptor_set(descriptor_set, find_layout_binding(bindings[i]), descriptor_infos[i]);
        }

        vkUpdateDescriptorSets(m_device, write.size(), write.data(), 0, nullptr);
}

}
