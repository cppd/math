/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ns::gpu::dft
{
class FftSharedMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int BUFFER_BINDING = 0;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        FftSharedMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set(const vulkan::Buffer& buffer) const;
};

class FftSharedProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
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

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline(bool inverse) const;
};
}
