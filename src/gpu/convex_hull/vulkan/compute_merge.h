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
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

#include <vector>

namespace gpu_vulkan
{
class ConvexHullMergeMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        ConvexHullMergeMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        ConvexHullMergeMemory(const ConvexHullMergeMemory&) = delete;
        ConvexHullMergeMemory& operator=(const ConvexHullMergeMemory&) = delete;
        ConvexHullMergeMemory& operator=(ConvexHullMergeMemory&&) = delete;

        ConvexHullMergeMemory(ConvexHullMergeMemory&&) = default;
        ~ConvexHullMergeMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_lines(const vulkan::BufferWithMemory& buffer) const;
};

class ConvexHullMergeConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t line_size;
                int32_t iteration_count;
                int32_t local_size_x;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        ConvexHullMergeConstant();

        void set_line_size(int32_t v);
        void set_iteration_count(int32_t v);
        void set_local_size_x(int32_t v);
};

class ConvexHullMergeProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        ConvexHullMergeConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        ConvexHullMergeProgram(const vulkan::Device& device);

        ConvexHullMergeProgram(const ConvexHullMergeProgram&) = delete;
        ConvexHullMergeProgram& operator=(const ConvexHullMergeProgram&) = delete;
        ConvexHullMergeProgram& operator=(ConvexHullMergeProgram&&) = delete;

        ConvexHullMergeProgram(ConvexHullMergeProgram&&) = default;
        ~ConvexHullMergeProgram() = default;

        void create_pipeline(unsigned height, unsigned local_size_x, unsigned iteration_count);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
