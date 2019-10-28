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
class DftMulMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int BUFFER_BINDING = 1;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        DftMulMemory(const vulkan::Device& device);

        DftMulMemory(const DftMulMemory&) = delete;
        DftMulMemory& operator=(const DftMulMemory&) = delete;
        DftMulMemory& operator=(DftMulMemory&&) = delete;

        DftMulMemory(DftMulMemory&&) = default;
        ~DftMulMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_data(const vulkan::BufferWithMemory& data) const;
        void set_buffer(const vulkan::BufferWithMemory& buffer) const;
};

class DftMulConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t function_index;
                int32_t n1;
                int32_t n2;
                int32_t m1;
                int32_t m2;
                uint32_t inverse;
                uint32_t group_size_x;
                uint32_t group_size_y;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        DftMulConstant();

        void set(int32_t function_index, int32_t n1, int32_t n2, int32_t m1, int32_t m2, bool inverse, uint32_t group_size_x,
                 uint32_t group_size_y);
};
}
