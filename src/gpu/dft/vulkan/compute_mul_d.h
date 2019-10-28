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
class DftMulDMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DIAGONAL_BINDING = 0;
        static constexpr int DATA_BINDING = 1;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        DftMulDMemory(const vulkan::Device& device);

        DftMulDMemory(const DftMulDMemory&) = delete;
        DftMulDMemory& operator=(const DftMulDMemory&) = delete;
        DftMulDMemory& operator=(DftMulDMemory&&) = delete;

        DftMulDMemory(DftMulDMemory&&) = default;
        ~DftMulDMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_diagonal(const vulkan::BufferWithMemory& diagonal) const;
        void set_data(const vulkan::BufferWithMemory& data) const;
};

class DftMulDConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t group_size_x;
                uint32_t group_size_y;
                int32_t rows;
                int32_t columns;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        DftMulDConstant();

        void set(uint32_t group_size_x, uint32_t group_size_y, int32_t rows, int32_t columns);
};
}
