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

#pragma once

#include "objects.h"

#include "com/variant.h"

#include <unordered_map>
#include <vector>

namespace vulkan
{
DescriptorSetLayout create_descriptor_set_layout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);

class Descriptors final
{
        VkDevice m_device;
        VkDescriptorSetLayout m_descriptor_set_layout;
        DescriptorPool m_descriptor_pool;
        std::vector<VkDescriptorSetLayoutBinding> m_descriptor_set_layout_bindings;
        DescriptorSets m_descriptor_sets;

        // VkDescriptorSetLayoutBinding::binding -> m_descriptor_set_layout_bindings index
        std::unordered_map<uint32_t, uint32_t> m_binding_map;

        // VkDescriptorSetLayoutBinding::binding -> const VkDescriptorSetLayoutBinding&
        const VkDescriptorSetLayoutBinding& layout_binding(uint32_t binding) const;

public:
        Descriptors(VkDevice device, uint32_t max_sets, VkDescriptorSetLayout descriptor_set_layout,
                    const std::vector<VkDescriptorSetLayoutBinding>& bindings);

        Descriptors(const Descriptors&) = delete;
        Descriptors& operator=(const Descriptors&) = delete;
        Descriptors& operator=(Descriptors&&) = delete;

        Descriptors(Descriptors&&) = default;
        ~Descriptors() = default;

        //

        uint32_t descriptor_set_count() const;
        const VkDescriptorSet& descriptor_set(uint32_t index) const;

        void update_descriptor_set(
                uint32_t index, const std::vector<uint32_t>& bindings,
                const std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>>& descriptor_infos) const;

        void update_descriptor_set(uint32_t index, uint32_t binding,
                                   const Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>& info) const;
};
}
