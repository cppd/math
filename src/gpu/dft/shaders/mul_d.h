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
class MulDMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DIAGONAL_BINDING = 0;
        static constexpr int DATA_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        MulDMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        MulDMemory(const MulDMemory&) = delete;
        MulDMemory& operator=(const MulDMemory&) = delete;
        MulDMemory& operator=(MulDMemory&&) = delete;

        MulDMemory(MulDMemory&&) = default;
        ~MulDMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set(const vulkan::BufferWithMemory& diagonal, const vulkan::BufferWithMemory& data) const;
};

class MulDConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t group_size_x;
                uint32_t group_size_y;
                int32_t rows;
                int32_t columns;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        MulDConstant();

        void set(uint32_t group_size_x, uint32_t group_size_y, int32_t rows, int32_t columns);
};

class MulDProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        MulDConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline_rows;
        vulkan::Pipeline m_pipeline_columns;

public:
        explicit MulDProgram(const vulkan::Device& device);

        MulDProgram(const MulDProgram&) = delete;
        MulDProgram& operator=(const MulDProgram&) = delete;
        MulDProgram& operator=(MulDProgram&&) = delete;

        MulDProgram(MulDProgram&&) = default;
        ~MulDProgram() = default;

        void create_pipelines(
                uint32_t n1,
                uint32_t n2,
                uint32_t m1,
                uint32_t m2,
                uint32_t group_size_x,
                uint32_t group_size_y);
        void delete_pipelines();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline_rows() const;
        VkPipeline pipeline_columns() const;
};
}
