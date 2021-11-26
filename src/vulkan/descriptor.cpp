/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/variant.h>

namespace ns::vulkan
{
namespace
{
vulkan::handle::DescriptorPool create_descriptor_pool(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        uint32_t max_sets,
        VkDescriptorPoolCreateFlags flags)
{
        std::vector<VkDescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(descriptor_set_layout_bindings.size());

        for (const VkDescriptorSetLayoutBinding& binding : descriptor_set_layout_bindings)
        {
                VkDescriptorPoolSize pool_size = {};
                pool_size.type = binding.descriptorType;
                pool_size.descriptorCount = max_sets * binding.descriptorCount;
                if (pool_size.descriptorCount > 0)
                {
                        pool_sizes.push_back(pool_size);
                }
        }

        if (pool_sizes.empty())
        {
                error("Empty descriptor pool sizes");
        }

        VkDescriptorPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.poolSizeCount = pool_sizes.size();
        create_info.pPoolSizes = pool_sizes.data();
        create_info.maxSets = max_sets;
        create_info.flags = flags;

        return vulkan::handle::DescriptorPool(device, create_info);
}

VkWriteDescriptorSet create_write_descriptor_set(
        VkDescriptorSet descriptor_set,
        const VkDescriptorSetLayoutBinding& descriptor_set_layout_binding,
        const std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>& descriptor_info)
{
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        write.dstSet = descriptor_set;
        write.dstBinding = descriptor_set_layout_binding.binding;
        write.dstArrayElement = 0;

        write.descriptorType = descriptor_set_layout_binding.descriptorType;
        write.descriptorCount = descriptor_set_layout_binding.descriptorCount;

        auto buffer = [&](const VkDescriptorBufferInfo& info)
        {
                ASSERT(descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                       || descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
                write.pBufferInfo = &info;
        };
        auto image = [&](const VkDescriptorImageInfo& info)
        {
                ASSERT(descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                       || descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
                write.pImageInfo = &info;
        };
        std::visit(Visitors{buffer, image}, descriptor_info);

        return write;

        // write.pTexelBufferView = nullptr;
}

std::unordered_map<uint32_t, uint32_t> create_binding_map(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
        std::unordered_map<uint32_t, uint32_t> map;
        for (std::size_t index = 0; index < bindings.size(); ++index)
        {
                if (!map.emplace(bindings[index].binding, index).second)
                {
                        error("Multiple binding " + to_string(bindings[index].binding)
                              + " in descriptor set layout bindings");
                }
        }
        return map;
}
}

handle::DescriptorSetLayout create_descriptor_set_layout(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
        VkDescriptorSetLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = bindings.size();
        create_info.pBindings = bindings.data();

        return handle::DescriptorSetLayout(device, create_info);
}

Descriptors::Descriptors(
        VkDevice device,
        uint32_t max_sets,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings)
        : device_(device),
          descriptor_set_layout_(descriptor_set_layout),
          descriptor_pool_(create_descriptor_pool(
                  device,
                  bindings,
                  max_sets,
                  VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)),
          descriptor_set_layout_bindings_(bindings),
          descriptor_sets_(
                  device_,
                  descriptor_pool_,
                  std::vector<VkDescriptorSetLayout>(max_sets, descriptor_set_layout_)),
          binding_map_(create_binding_map(descriptor_set_layout_bindings_))
{
}

const VkDescriptorSetLayoutBinding& Descriptors::layout_binding(uint32_t binding) const
{
        auto i = binding_map_.find(binding);
        if (i == binding_map_.cend())
        {
                error("No binding " + to_string(binding) + " in the descriptor set layout bindings");
        }

        ASSERT(i->second < descriptor_set_layout_bindings_.size());

        return descriptor_set_layout_bindings_[i->second];
}

VkDescriptorSetLayout Descriptors::descriptor_set_layout() const noexcept
{
        return descriptor_set_layout_;
}

uint32_t Descriptors::descriptor_set_count() const
{
        return descriptor_sets_.count();
}

const VkDescriptorSet& Descriptors::descriptor_set(uint32_t index) const
{
        ASSERT(index < descriptor_sets_.count());

        return descriptor_sets_[index];
}

void Descriptors::update_descriptor_set(
        uint32_t index,
        uint32_t binding,
        const std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>& info) const
{
        ASSERT(index < descriptor_sets_.count());

        VkWriteDescriptorSet write =
                create_write_descriptor_set(descriptor_sets_[index], layout_binding(binding), info);

        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void Descriptors::update_descriptor_set(
        uint32_t index,
        const std::vector<uint32_t>& bindings,
        const std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>>& descriptor_infos) const
{
        ASSERT(bindings.size() == descriptor_infos.size());
        ASSERT(index < descriptor_sets_.count());

        VkDescriptorSet descriptor_set = descriptor_sets_[index];

        std::vector<VkWriteDescriptorSet> write(bindings.size());
        for (std::size_t i = 0; i < bindings.size(); ++i)
        {
                write[i] =
                        create_write_descriptor_set(descriptor_set, layout_binding(bindings[i]), descriptor_infos[i]);
        }

        vkUpdateDescriptorSets(device_, write.size(), write.data(), 0, nullptr);
}
}
