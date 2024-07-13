/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::dft
{
class FftGlobalBuffer final
{
        vulkan::BufferWithMemory buffer_;

        struct Data final
        {
                std::uint32_t m_div_2;
                float two_pi_div_m;
        };

public:
        FftGlobalBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        [[nodiscard]] const vulkan::Buffer& buffer() const;

        void set(float two_pi_div_m, int m_div_2) const;
};

class FftGlobalMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int BUFFER_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        FftGlobalMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, const vulkan::Buffer& buffer);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set(const vulkan::Buffer& buffer) const;
};

class FftGlobalProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
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

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline(bool inverse) const;
};
}
