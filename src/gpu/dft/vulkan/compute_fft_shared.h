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
class DftFftSharedMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int BUFFER_BINDING = 0;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        DftFftSharedMemory(const vulkan::Device& device);

        DftFftSharedMemory(const DftFftSharedMemory&) = delete;
        DftFftSharedMemory& operator=(const DftFftSharedMemory&) = delete;
        DftFftSharedMemory& operator=(DftFftSharedMemory&&) = delete;

        DftFftSharedMemory(DftFftSharedMemory&&) = default;
        ~DftFftSharedMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_buffer(const vulkan::BufferWithMemory& buffer) const;
};

class DftFftSharedConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t inverse;
                uint32_t data_size;
                uint32_t n;
                uint32_t n_mask;
                uint32_t n_bits;
                uint32_t shared_size;
                uint32_t reverse_input;
                uint32_t group_size;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        DftFftSharedConstant();

        void set(bool inverse, uint32_t data_size, uint32_t n, uint32_t n_mask, uint32_t n_bits, uint32_t shared_size,
                 bool reverse_input, uint32_t group_size);
};
}
