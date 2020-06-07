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

#include <src/vulkan/buffers.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace gpu::dft
{
class MulMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int BUFFER_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        MulMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        MulMemory(const MulMemory&) = delete;
        MulMemory& operator=(const MulMemory&) = delete;
        MulMemory& operator=(MulMemory&&) = delete;

        MulMemory(MulMemory&&) = default;
        ~MulMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set(const vulkan::BufferWithMemory& data, const vulkan::BufferWithMemory& buffer) const;
};

class MulConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t function_index;
                int32_t n1;
                int32_t n2;
                int32_t m1;
                int32_t m2;
                uint32_t inverse;
                uint32_t group_size_x;
                uint32_t group_size_y;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        MulConstant();

        void set_data(int32_t n1, int32_t n2, int32_t m1, int32_t m2, uint32_t group_size_x, uint32_t group_size_y);
        void set_function(int32_t function_index, bool inverse);
};

class MulProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        MulConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline_rows_to_buffer_forward;
        vulkan::Pipeline m_pipeline_rows_to_buffer_inverse;
        vulkan::Pipeline m_pipeline_rows_from_buffer_forward;
        vulkan::Pipeline m_pipeline_rows_from_buffer_inverse;
        vulkan::Pipeline m_pipeline_columns_to_buffer_forward;
        vulkan::Pipeline m_pipeline_columns_to_buffer_inverse;
        vulkan::Pipeline m_pipeline_columns_from_buffer_forward;
        vulkan::Pipeline m_pipeline_columns_from_buffer_inverse;

public:
        explicit MulProgram(const vulkan::Device& device);

        MulProgram(const MulProgram&) = delete;
        MulProgram& operator=(const MulProgram&) = delete;
        MulProgram& operator=(MulProgram&&) = delete;

        MulProgram(MulProgram&&) = default;
        ~MulProgram() = default;

        void create_pipelines(
                int32_t n1,
                int32_t n2,
                int32_t m1,
                int32_t m2,
                uint32_t group_size_x,
                uint32_t group_size_y);
        void delete_pipelines();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline_rows_to_buffer(bool inverse) const;
        VkPipeline pipeline_rows_from_buffer(bool inverse) const;
        VkPipeline pipeline_columns_to_buffer(bool inverse) const;
        VkPipeline pipeline_columns_from_buffer(bool inverse) const;
};
}
