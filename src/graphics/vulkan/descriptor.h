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

#pragma once

#include "objects.h"

#include "com/variant.h"

#include <vector>

namespace vulkan
{
DescriptorSetLayout create_descriptor_set_layout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);

class Descriptors
{
        VkDevice m_device;
        VkDescriptorSetLayout m_descriptor_set_layout;
        DescriptorPool m_descriptor_pool;
        std::vector<VkDescriptorSetLayoutBinding> m_descriptor_set_layout_bindings;

public:
        Descriptors(VkDevice device, uint32_t max_sets, VkDescriptorSetLayout descriptor_set_layout,
                    const std::vector<VkDescriptorSetLayoutBinding>& bindings);

        DescriptorSet create_descriptor_set(
                const std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>>& descriptor_infos);
};
}
