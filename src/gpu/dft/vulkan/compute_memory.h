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
class DftCopyInputMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int SRC_BINDING = 1;
        static constexpr int DST_BINDING = 0;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        DftCopyInputMemory(const vulkan::Device& device);

        DftCopyInputMemory(const DftCopyInputMemory&) = delete;
        DftCopyInputMemory& operator=(const DftCopyInputMemory&) = delete;
        DftCopyInputMemory& operator=(DftCopyInputMemory&&) = delete;

        DftCopyInputMemory(DftCopyInputMemory&&) = default;
        ~DftCopyInputMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_input(VkSampler sampler, const vulkan::ImageWithMemory& image) const;
        void set_output(const vulkan::BufferWithMemory& buffer) const;
};

class DftCopyInputConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t local_size_x;
                uint32_t local_size_y;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        DftCopyInputConstant();

        void set_group_size(uint32_t x, uint32_t y);
};

class DftCopyOutputMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int SRC_BINDING = 0;
        static constexpr int DST_BINDING = 1;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        DftCopyOutputMemory(const vulkan::Device& device);

        DftCopyOutputMemory(const DftCopyOutputMemory&) = delete;
        DftCopyOutputMemory& operator=(const DftCopyOutputMemory&) = delete;
        DftCopyOutputMemory& operator=(DftCopyOutputMemory&&) = delete;

        DftCopyOutputMemory(DftCopyOutputMemory&&) = default;
        ~DftCopyOutputMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_input(const vulkan::BufferWithMemory& buffer) const;
        void set_output(const vulkan::ImageWithMemory& image) const;
};

class DftCopyOutputConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t local_size_x;
                uint32_t local_size_y;
                float to_mul;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        DftCopyOutputConstant();

        void set_group_size(uint32_t x, uint32_t y);
        void set_to_mul(float v);
};

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

//

class DftFftGlobalMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int BUFFER_BINDING = 1;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct Data
        {
                uint32_t data_size;
                uint32_t n_div_2_mask;
                uint32_t m_div_2;
                float two_pi_div_m;
        };

public:
        DftFftGlobalMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        DftFftGlobalMemory(const DftFftGlobalMemory&) = delete;
        DftFftGlobalMemory& operator=(const DftFftGlobalMemory&) = delete;
        DftFftGlobalMemory& operator=(DftFftGlobalMemory&&) = delete;

        DftFftGlobalMemory(DftFftGlobalMemory&&) = default;
        ~DftFftGlobalMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_data(int data_size, float two_pi_div_m, int n_div_2_mask, int m_div_2) const;
        void set_buffer(const vulkan::BufferWithMemory& buffer) const;
};

class DftFftGlobalConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t group_size;
                uint32_t inverse;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        DftFftGlobalConstant();

        void set(uint32_t group_size, bool inverse);
};

//

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
