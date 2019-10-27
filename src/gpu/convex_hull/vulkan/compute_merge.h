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
class ConvexHullMergeMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        ConvexHullMergeMemory(const vulkan::Device& device);

        ConvexHullMergeMemory(const ConvexHullMergeMemory&) = delete;
        ConvexHullMergeMemory& operator=(const ConvexHullMergeMemory&) = delete;
        ConvexHullMergeMemory& operator=(ConvexHullMergeMemory&&) = delete;

        ConvexHullMergeMemory(ConvexHullMergeMemory&&) = default;
        ~ConvexHullMergeMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
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

class ConvexHullProgramMerge final
{
        const vulkan::VulkanInstance& m_instance;

        ConvexHullMergeMemory m_memory;
        ConvexHullMergeConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

public:
        ConvexHullProgramMerge(const vulkan::VulkanInstance& instance);

        ConvexHullProgramMerge(const ConvexHullProgramMerge&) = delete;
        ConvexHullProgramMerge& operator=(const ConvexHullProgramMerge&) = delete;
        ConvexHullProgramMerge& operator=(ConvexHullProgramMerge&&) = delete;

        ConvexHullProgramMerge(ConvexHullProgramMerge&&) = default;
        ~ConvexHullProgramMerge() = default;

        void create_buffers(unsigned height, const vulkan::BufferWithMemory& lines_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
};
}
