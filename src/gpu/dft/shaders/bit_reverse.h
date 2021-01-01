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

#pragma once

#include <src/vulkan/buffers.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::dft
{
class BitReverseMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int BUFFER_BINDING = 0;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        BitReverseMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        BitReverseMemory(const BitReverseMemory&) = delete;
        BitReverseMemory& operator=(const BitReverseMemory&) = delete;
        BitReverseMemory& operator=(BitReverseMemory&&) = delete;

        BitReverseMemory(BitReverseMemory&&) = default;
        ~BitReverseMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_buffer(const vulkan::BufferWithMemory& buffer) const;
};

class BitReverseConstant final : public vulkan::SpecializationConstant
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
        std::size_t size() const override;

public:
        BitReverseConstant();

        void set(uint32_t group_size, uint32_t data_size, uint32_t n_mask, uint32_t n_bits);
};

class BitReverseProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        BitReverseConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        explicit BitReverseProgram(const vulkan::Device& device);

        BitReverseProgram(const BitReverseProgram&) = delete;
        BitReverseProgram& operator=(const BitReverseProgram&) = delete;
        BitReverseProgram& operator=(BitReverseProgram&&) = delete;

        BitReverseProgram(BitReverseProgram&&) = default;
        ~BitReverseProgram() = default;

        void create_pipeline(uint32_t group_size, uint32_t data_size, uint32_t n_mask, uint32_t n_bits);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
