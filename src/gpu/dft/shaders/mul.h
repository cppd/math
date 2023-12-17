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

namespace ns::gpu::dft
{
class MulMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int BUFFER_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        MulMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set(const vulkan::Buffer& data, const vulkan::Buffer& buffer) const;
};

class MulProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_rows_to_buffer_forward_;
        vulkan::handle::Pipeline pipeline_rows_to_buffer_inverse_;
        vulkan::handle::Pipeline pipeline_rows_from_buffer_forward_;
        vulkan::handle::Pipeline pipeline_rows_from_buffer_inverse_;
        vulkan::handle::Pipeline pipeline_columns_to_buffer_forward_;
        vulkan::handle::Pipeline pipeline_columns_to_buffer_inverse_;
        vulkan::handle::Pipeline pipeline_columns_from_buffer_forward_;
        vulkan::handle::Pipeline pipeline_columns_from_buffer_inverse_;

public:
        explicit MulProgram(VkDevice device);

        MulProgram(const MulProgram&) = delete;
        MulProgram& operator=(const MulProgram&) = delete;
        MulProgram& operator=(MulProgram&&) = delete;

        MulProgram(MulProgram&&) = default;
        ~MulProgram() = default;

        void create_pipelines(
                std::int32_t n_1,
                std::int32_t n_2,
                std::int32_t m_1,
                std::int32_t m_2,
                std::uint32_t group_size_x,
                std::uint32_t group_size_y);
        void delete_pipelines();

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline_rows_to_buffer(bool inverse) const;
        [[nodiscard]] VkPipeline pipeline_rows_from_buffer(bool inverse) const;
        [[nodiscard]] VkPipeline pipeline_columns_to_buffer(bool inverse) const;
        [[nodiscard]] VkPipeline pipeline_columns_from_buffer(bool inverse) const;
};
}
