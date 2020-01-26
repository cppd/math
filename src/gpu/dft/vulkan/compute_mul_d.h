/*
Copyright (C) 2017-2020 Topological Manifold

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
#include "graphics/vulkan/shader.h"

#include <vector>

namespace gpu_vulkan
{
class DftMulDMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DIAGONAL_BINDING = 0;
        static constexpr int DATA_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        DftMulDMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        DftMulDMemory(const DftMulDMemory&) = delete;
        DftMulDMemory& operator=(const DftMulDMemory&) = delete;
        DftMulDMemory& operator=(DftMulDMemory&&) = delete;

        DftMulDMemory(DftMulDMemory&&) = default;
        ~DftMulDMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set(const vulkan::BufferWithMemory& diagonal, const vulkan::BufferWithMemory& data) const;
};

class DftMulDConstant final : public vulkan::SpecializationConstant
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
        size_t size() const override;

public:
        DftMulDConstant();

        void set(uint32_t group_size_x, uint32_t group_size_y, int32_t rows, int32_t columns);
};

class DftMulDProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        DftMulDConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline_rows;
        vulkan::Pipeline m_pipeline_columns;

public:
        explicit DftMulDProgram(const vulkan::Device& device);

        DftMulDProgram(const DftMulDProgram&) = delete;
        DftMulDProgram& operator=(const DftMulDProgram&) = delete;
        DftMulDProgram& operator=(DftMulDProgram&&) = delete;

        DftMulDProgram(DftMulDProgram&&) = default;
        ~DftMulDProgram() = default;

        void create_pipelines(uint32_t n1, uint32_t n2, uint32_t m1, uint32_t m2, uint32_t group_size_x, uint32_t group_size_y);
        void delete_pipelines();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline_rows() const;
        VkPipeline pipeline_columns() const;
};
}
