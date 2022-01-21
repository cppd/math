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

#include <src/vulkan/buffers.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::dft
{
class FftGlobalMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int BUFFER_BINDING = 1;

        vulkan::Descriptors descriptors_;
        std::vector<vulkan::BufferWithMemory> uniform_buffers_;

        struct Data
        {
                std::uint32_t m_div_2;
                float two_pi_div_m;
        };

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        FftGlobalMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<std::uint32_t>& family_indices);

        FftGlobalMemory(const FftGlobalMemory&) = delete;
        FftGlobalMemory& operator=(const FftGlobalMemory&) = delete;
        FftGlobalMemory& operator=(FftGlobalMemory&&) = delete;

        FftGlobalMemory(FftGlobalMemory&&) = default;
        ~FftGlobalMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_data(float two_pi_div_m, int m_div_2) const;
        void set_buffer(const vulkan::Buffer& buffer) const;
};

class FftGlobalConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                std::uint32_t group_size;
                std::uint32_t inverse;
                std::uint32_t data_size;
                std::uint32_t n;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        FftGlobalConstant();

        void set(std::uint32_t group_size, bool inverse, std::uint32_t data_size, std::uint32_t n);
};

class FftGlobalProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        FftGlobalConstant constant_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_forward_;
        vulkan::handle::Pipeline pipeline_inverse_;

public:
        explicit FftGlobalProgram(VkDevice device);

        FftGlobalProgram(const FftGlobalProgram&) = delete;
        FftGlobalProgram& operator=(const FftGlobalProgram&) = delete;
        FftGlobalProgram& operator=(FftGlobalProgram&&) = delete;

        FftGlobalProgram(FftGlobalProgram&&) = default;
        ~FftGlobalProgram() = default;

        void create_pipelines(std::uint32_t group_size, std::uint32_t data_size, std::uint32_t n);
        void delete_pipelines();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline(bool inverse) const;
};
}
