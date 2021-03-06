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

namespace ns::gpu::convex_hull
{
class FilterMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;
        static constexpr int POINT_COUNT_BINDING = 2;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        FilterMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        FilterMemory(const FilterMemory&) = delete;
        FilterMemory& operator=(const FilterMemory&) = delete;
        FilterMemory& operator=(FilterMemory&&) = delete;

        FilterMemory(FilterMemory&&) = default;
        ~FilterMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_lines(const vulkan::BufferWithMemory& buffer) const;
        void set_points(const vulkan::BufferWithMemory& buffer) const;
        void set_point_count(const vulkan::BufferWithMemory& buffer) const;
};

class FilterConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t line_size;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        FilterConstant();

        void set_line_size(int32_t v);
};

class FilterProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        FilterConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        explicit FilterProgram(const vulkan::Device& device);

        FilterProgram(const FilterProgram&) = delete;
        FilterProgram& operator=(const FilterProgram&) = delete;
        FilterProgram& operator=(FilterProgram&&) = delete;

        FilterProgram(FilterProgram&&) = default;
        ~FilterProgram() = default;

        void create_pipeline(unsigned height);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
