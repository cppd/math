/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "objects.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/variant.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ns::vulkan
{
namespace
{
handle::DescriptorPool create_descriptor_pool(
        const VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const std::uint32_t max_sets,
        const VkDescriptorPoolCreateFlags flags)
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

        VkDescriptorPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.poolSizeCount = pool_sizes.size();
        info.pPoolSizes = pool_sizes.data();
        info.maxSets = max_sets;
        info.flags = flags;

        return {device, info};
}

void write_descriptor_set(
        const VkDescriptorSet descriptor_set,
        const VkDescriptorSetLayoutBinding& descriptor_set_layout_binding,
        const Descriptors::Info& info,
        VkWriteDescriptorSet* const write,
        VkWriteDescriptorSetAccelerationStructureKHR* const write_as)
{
        *write = {};
        write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write->dstSet = descriptor_set;
        write->dstBinding = descriptor_set_layout_binding.binding;
        write->dstArrayElement = 0;
        write->descriptorType = descriptor_set_layout_binding.descriptorType;
        write->descriptorCount = descriptor_set_layout_binding.descriptorCount;
        ASSERT(write->descriptorCount == 1);

        const auto visitors = Visitors{
                [&](const VkDescriptorBufferInfo& buffer_info)
                {
                        ASSERT(descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                               || descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
                        write->pBufferInfo = &buffer_info;
                },
                [&](const VkDescriptorImageInfo& image_info)
                {
                        ASSERT(descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                               || descriptor_set_layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
                        write->pImageInfo = &image_info;
                },
                [&](const VkAccelerationStructureKHR& acceleration_structure)
                {
                        ASSERT(descriptor_set_layout_binding.descriptorType
                               == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
                        *write_as = {};
                        write_as->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                        write_as->accelerationStructureCount = 1;
                        write_as->pAccelerationStructures = &acceleration_structure;
                        write->pNext = write_as;
                }};

        std::visit(visitors, info);
}

std::unordered_map<std::uint32_t, std::uint32_t> create_binding_map(
        const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
        std::unordered_map<std::uint32_t, std::uint32_t> res;
        for (std::size_t index = 0; index < bindings.size(); ++index)
        {
                if (!res.emplace(bindings[index].binding, index).second)
                {
                        error("Multiple binding " + to_string(bindings[index].binding)
                              + " in descriptor set layout bindings");
                }
        }
        return res;
}
}

handle::DescriptorSetLayout create_descriptor_set_layout(
        const VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = bindings.size();
        info.pBindings = bindings.data();

        return {device, info};
}

Descriptors::Descriptors(
        const VkDevice device,
        const std::uint32_t max_sets,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings)
        : device_(device),
          descriptor_set_layout_(descriptor_set_layout),
          descriptor_pool_(create_descriptor_pool(
                  device,
                  bindings,
                  max_sets,
                  VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)),
          descriptor_set_layout_bindings_(bindings),
          descriptor_sets_(device_, descriptor_pool_, std::vector(max_sets, descriptor_set_layout_)),
          binding_map_(create_binding_map(descriptor_set_layout_bindings_))
{
}

const VkDescriptorSetLayoutBinding& Descriptors::layout_binding(const std::uint32_t binding) const
{
        const auto i = binding_map_.find(binding);
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

std::uint32_t Descriptors::descriptor_set_count() const
{
        return descriptor_sets_.count();
}

const VkDescriptorSet& Descriptors::descriptor_set(const std::uint32_t index) const
{
        ASSERT(index < descriptor_sets_.count());

        return descriptor_sets_[index];
}

void Descriptors::update_descriptor_set(const std::uint32_t index, const std::uint32_t binding, const Info& info) const
{
        ASSERT(index < descriptor_sets_.count());

        VkWriteDescriptorSet write;
        VkWriteDescriptorSetAccelerationStructureKHR write_as;
        write_descriptor_set(descriptor_sets_[index], layout_binding(binding), info, &write, &write_as);

        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void Descriptors::update_descriptor_set(const std::vector<DescriptorInfo>& infos) const
{
        std::vector<VkWriteDescriptorSet> write(infos.size());
        std::vector<VkWriteDescriptorSetAccelerationStructureKHR> write_as(infos.size());
        for (std::size_t i = 0; i < infos.size(); ++i)
        {
                const DescriptorInfo& info = infos[i];
                ASSERT(info.index < descriptor_sets_.count());
                write_descriptor_set(
                        descriptor_sets_[info.index], layout_binding(info.binding), info.info, &write[i], &write_as[i]);
        }

        vkUpdateDescriptorSets(device_, write.size(), write.data(), 0, nullptr);
}
}
