/*
Copyright (C) 2017-2019 Topological Manifold

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
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

#include <vector>

namespace gpu_vulkan
{
class ConvexHullFilterMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;
        static constexpr int POINT_COUNT_BINDING = 2;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        ConvexHullFilterMemory(const vulkan::Device& device);

        ConvexHullFilterMemory(const ConvexHullFilterMemory&) = delete;
        ConvexHullFilterMemory& operator=(const ConvexHullFilterMemory&) = delete;
        ConvexHullFilterMemory& operator=(ConvexHullFilterMemory&&) = delete;

        ConvexHullFilterMemory(ConvexHullFilterMemory&&) = default;
        ~ConvexHullFilterMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
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

class ConvexHullProgramFilter final
{
        const vulkan::VulkanInstance& m_instance;

        ConvexHullFilterMemory m_memory;
        ConvexHullFilterConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

public:
        ConvexHullProgramFilter(const vulkan::VulkanInstance& instance);

        ConvexHullProgramFilter(const ConvexHullProgramFilter&) = delete;
        ConvexHullProgramFilter& operator=(const ConvexHullProgramFilter&) = delete;
        ConvexHullProgramFilter& operator=(ConvexHullProgramFilter&&) = delete;

        ConvexHullProgramFilter(ConvexHullProgramFilter&&) = default;
        ~ConvexHullProgramFilter() = default;

        void create_buffers(unsigned height, const vulkan::BufferWithMemory& lines_buffer,
                            const vulkan::BufferWithMemory& points_buffer, const vulkan::BufferWithMemory& point_count_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
};
}
