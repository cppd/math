/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::dft
{
class FftSharedMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int BUFFER_BINDING = 0;

        vulkan::Descriptors descriptors_;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        FftSharedMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        FftSharedMemory(const FftSharedMemory&) = delete;
        FftSharedMemory& operator=(const FftSharedMemory&) = delete;
        FftSharedMemory& operator=(FftSharedMemory&&) = delete;

        FftSharedMemory(FftSharedMemory&&) = default;
        ~FftSharedMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_buffer(const vulkan::Buffer& buffer) const;
};

class FftSharedConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                std::uint32_t inverse;
                std::uint32_t data_size;
                std::uint32_t n;
                std::uint32_t n_mask;
                std::uint32_t n_bits;
                std::uint32_t shared_size;
                std::uint32_t reverse_input;
                std::uint32_t group_size;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        FftSharedConstant();

        void set(
                bool inverse,
                std::uint32_t data_size,
                std::uint32_t n,
                std::uint32_t n_mask,
                std::uint32_t n_bits,
                std::uint32_t shared_size,
                bool reverse_input,
                std::uint32_t group_size);
};

class FftSharedProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        FftSharedConstant constant_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_forward_;
        vulkan::handle::Pipeline pipeline_inverse_;

public:
        explicit FftSharedProgram(VkDevice device);

        FftSharedProgram(const FftSharedProgram&) = delete;
        FftSharedProgram& operator=(const FftSharedProgram&) = delete;
        FftSharedProgram& operator=(FftSharedProgram&&) = delete;

        FftSharedProgram(FftSharedProgram&&) = default;
        ~FftSharedProgram() = default;

        void create_pipelines(
                std::uint32_t data_size,
                std::uint32_t n,
                std::uint32_t n_mask,
                std::uint32_t n_bits,
                std::uint32_t shared_size,
                bool reverse_input,
                std::uint32_t group_size);
        void delete_pipelines();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline(bool inverse) const;
};
}
