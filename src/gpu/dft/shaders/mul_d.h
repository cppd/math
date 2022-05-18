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
class MulDMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DIAGONAL_BINDING = 0;
        static constexpr int DATA_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        MulDMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set(const vulkan::Buffer& diagonal, const vulkan::Buffer& data) const;
};

class MulDConstant final : public vulkan::SpecializationConstant
{
        struct Data final
        {
                std::uint32_t group_size_x;
                std::uint32_t group_size_y;
                std::int32_t rows;
                std::int32_t columns;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        [[nodiscard]] const std::vector<VkSpecializationMapEntry>& entries() const override;
        [[nodiscard]] const void* data() const override;
        [[nodiscard]] std::size_t size() const override;

public:
        MulDConstant();

        void set(std::uint32_t group_size_x, std::uint32_t group_size_y, std::int32_t rows, std::int32_t columns);
};

class MulDProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        MulDConstant constant_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_rows_;
        vulkan::handle::Pipeline pipeline_columns_;

public:
        explicit MulDProgram(VkDevice device);

        MulDProgram(const MulDProgram&) = delete;
        MulDProgram& operator=(const MulDProgram&) = delete;
        MulDProgram& operator=(MulDProgram&&) = delete;

        MulDProgram(MulDProgram&&) = default;
        ~MulDProgram() = default;

        void create_pipelines(
                std::uint32_t n1,
                std::uint32_t n2,
                std::uint32_t m1,
                std::uint32_t m2,
                std::uint32_t group_size_x,
                std::uint32_t group_size_y);
        void delete_pipelines();

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline_rows() const;
        [[nodiscard]] VkPipeline pipeline_columns() const;
};
}
