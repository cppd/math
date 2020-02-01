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

#include <src/graphics/vulkan/buffers.h>
#include <src/graphics/vulkan/constant.h>
#include <src/graphics/vulkan/descriptor.h>
#include <src/graphics/vulkan/objects.h>
#include <src/graphics/vulkan/shader.h>

#include <vector>

namespace gpu_vulkan
{
class ConvexHullFilterMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;
        static constexpr int POINT_COUNT_BINDING = 2;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        ConvexHullFilterMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        ConvexHullFilterMemory(const ConvexHullFilterMemory&) = delete;
        ConvexHullFilterMemory& operator=(const ConvexHullFilterMemory&) = delete;
        ConvexHullFilterMemory& operator=(ConvexHullFilterMemory&&) = delete;

        ConvexHullFilterMemory(ConvexHullFilterMemory&&) = default;
        ~ConvexHullFilterMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_lines(const vulkan::BufferWithMemory& buffer) const;
        void set_points(const vulkan::BufferWithMemory& buffer) const;
        void set_point_count(const vulkan::BufferWithMemory& buffer) const;
};

class ConvexHullFilterConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t line_size;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        ConvexHullFilterConstant();

        void set_line_size(int32_t v);
};

class ConvexHullFilterProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        ConvexHullFilterConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        explicit ConvexHullFilterProgram(const vulkan::Device& device);

        ConvexHullFilterProgram(const ConvexHullFilterProgram&) = delete;
        ConvexHullFilterProgram& operator=(const ConvexHullFilterProgram&) = delete;
        ConvexHullFilterProgram& operator=(ConvexHullFilterProgram&&) = delete;

        ConvexHullFilterProgram(ConvexHullFilterProgram&&) = default;
        ~ConvexHullFilterProgram() = default;

        void create_pipeline(unsigned height);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
