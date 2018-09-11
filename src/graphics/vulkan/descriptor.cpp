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

vulkan::DescriptorSet create_descriptor_set(VkDevice device, VkDescriptorPool descriptor_pool,
                                            VkDescriptorSetLayout descriptor_set_layout,
                                            const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                                            const std::vector<VkDescriptorBufferInfo>& descriptor_buffer_infos,
                                            const std::vector<VkDescriptorImageInfo>& descriptor_image_infos)
{
        ASSERT(descriptor_set_layout_bindings.size() == descriptor_buffer_infos.size() + descriptor_image_infos.size());

        vulkan::DescriptorSet descriptor_set(device, descriptor_pool, descriptor_set_layout);

        const uint32_t size = descriptor_set_layout_bindings.size();

        std::vector<VkWriteDescriptorSet> write_descriptor_set(size);

        for (uint32_t i = 0, buffer_i = 0, image_i = 0; i < size; ++i)
        {
                write_descriptor_set[i] = {};
                write_descriptor_set[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write_descriptor_set[i].dstSet = descriptor_set;
                write_descriptor_set[i].dstBinding = descriptor_set_layout_bindings[i].binding;
                write_descriptor_set[i].dstArrayElement = 0;

                write_descriptor_set[i].descriptorType = descriptor_set_layout_bindings[i].descriptorType;
                write_descriptor_set[i].descriptorCount = descriptor_set_layout_bindings[i].descriptorCount;

                if (descriptor_set_layout_bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                {
                        ASSERT(buffer_i < descriptor_buffer_infos.size());
                        write_descriptor_set[i].pBufferInfo = &descriptor_buffer_infos[buffer_i++];
                }
                else if (descriptor_set_layout_bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                        ASSERT(image_i < descriptor_image_infos.size());
                        write_descriptor_set[i].pImageInfo = &descriptor_image_infos[image_i++];
                }
                else
                {
                        error_fatal("Not supported descriptor type");
                }
                // write_descriptor_set[i].pTexelBufferView = nullptr;
        }

        vkUpdateDescriptorSets(device, write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);

        return descriptor_set;
}
}

namespace vulkan
{
Descriptors::Descriptors() = default;

Descriptors::Descriptors(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                         const std::vector<VkDescriptorBufferInfo>& descriptor_buffer_infos,
                         const std::vector<VkDescriptorImageInfo>& descriptor_image_infos)
        : m_descriptor_set_layout(create_descriptor_set_layout(device, descriptor_set_layout_bindings)),
          m_descriptor_pool(create_descriptor_pool(device, descriptor_set_layout_bindings, 1 /*max_sets*/,
                                                   VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)),
          m_descriptor_set(create_descriptor_set(device, m_descriptor_pool, m_descriptor_set_layout,
                                                 descriptor_set_layout_bindings, descriptor_buffer_infos, descriptor_image_infos))
{
}

VkDescriptorSetLayout Descriptors::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

VkDescriptorSet Descriptors::descriptor_set() const noexcept
{
        return m_descriptor_set;
}
}
