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

#include "descriptor.h"

#include "com/error.h"

namespace
{
template <typename... T>
struct Visitors : T...
{
        using T::operator()...;
};
template <typename... T>
Visitors(T...)->Visitors<T...>;

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
        const std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>>& descriptor_infos)
{
        ASSERT(descriptor_set_layout_bindings.size() == descriptor_infos.size());

        vulkan::DescriptorSet descriptor_set(device, descriptor_pool, descriptor_set_layout);

        const uint32_t size = descriptor_set_layout_bindings.size();

        std::vector<VkWriteDescriptorSet> write_descriptor_set(size);

        for (uint32_t i = 0; i < size; ++i)
        {
                write_descriptor_set[i] = {};
                write_descriptor_set[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write_descriptor_set[i].dstSet = descriptor_set;
                write_descriptor_set[i].dstBinding = descriptor_set_layout_bindings[i].binding;
                write_descriptor_set[i].dstArrayElement = 0;

                write_descriptor_set[i].descriptorType = descriptor_set_layout_bindings[i].descriptorType;
                write_descriptor_set[i].descriptorCount = descriptor_set_layout_bindings[i].descriptorCount;

                auto buffer = [&](const VkDescriptorBufferInfo& info) noexcept
                {
                        ASSERT(descriptor_set_layout_bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                        write_descriptor_set[i].pBufferInfo = &info;
                };
                auto image = [&](const VkDescriptorImageInfo& info) noexcept
                {
                        ASSERT(descriptor_set_layout_bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                        write_descriptor_set[i].pImageInfo = &info;
                };
                visit(Visitors{buffer, image}, descriptor_infos[i]);

                // write_descriptor_set[i].pTexelBufferView = nullptr;
        }

        vkUpdateDescriptorSets(device, write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);

        return descriptor_set;
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

Descriptors::Descriptors() = default;

Descriptors::Descriptors(VkDevice device, uint32_t max_sets, VkDescriptorSetLayout descriptor_set_layout,
                         const std::vector<VkDescriptorSetLayoutBinding>& bindings)
        : m_device(device),
          m_descriptor_set_layout(descriptor_set_layout),
          m_descriptor_pool(
                  create_descriptor_pool(device, bindings, max_sets, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)),
          m_descriptor_set_layout_bindings(bindings)
{
        ASSERT(descriptor_set_layout != VK_NULL_HANDLE);
}

DescriptorSet Descriptors::create_descriptor_set(
        const std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>>& descriptor_infos)
{
        return ::create_descriptor_set(m_device, m_descriptor_pool, m_descriptor_set_layout, m_descriptor_set_layout_bindings,
                                       descriptor_infos);
}
}
