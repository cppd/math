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

#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/constant.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"

#include <vector>

namespace gpu_vulkan
{
class DftBitReverseMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int BUFFER_BINDING = 0;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        DftBitReverseMemory(const vulkan::Device& device);

        DftBitReverseMemory(const DftBitReverseMemory&) = delete;
        DftBitReverseMemory& operator=(const DftBitReverseMemory&) = delete;
        DftBitReverseMemory& operator=(DftBitReverseMemory&&) = delete;

        DftBitReverseMemory(DftBitReverseMemory&&) = default;
        ~DftBitReverseMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_buffer(const vulkan::BufferWithMemory& buffer) const;
};

class DftBitReverseConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t group_size;
                uint32_t data_size;
                uint32_t n_mask;
                uint32_t n_bits;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        DftBitReverseConstant();

        void set(uint32_t group_size, uint32_t data_size, uint32_t n_mask, uint32_t n_bits);
};
}
