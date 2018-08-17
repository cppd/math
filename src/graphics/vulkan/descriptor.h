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

#if defined(VULKAN_FOUND)

#include "buffers.h"
#include "objects.h"

#include "com/error.h"
#include "com/span.h"

#include <vector>

namespace vulkan
{
class DescriptorWithBuffers
{
        DescriptorSetLayout m_descriptor_set_layout;
        std::vector<UniformBufferWithHostVisibleMemory> m_descriptor_set_layout_uniform_buffers;
        DescriptorPool m_descriptor_pool;
        DescriptorSet m_descriptor_set;

public:
        DescriptorWithBuffers(const vulkan::Device& device,
                              const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                              const std::vector<VkDeviceSize>& descriptor_set_layout_bindings_sizes);

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        VkDescriptorSet descriptor_set() const noexcept;

        void copy_to_buffer(uint32_t index, const Span<const void>& data) const;
};
}

#endif
