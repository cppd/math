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

#include "compute_memory.h"

#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

namespace gpgpu_vulkan
{
class ConvexHullProgramPrepare final
{
        const vulkan::VulkanInstance& m_instance;

        ConvexHullPrepareMemory m_memory;
        ConvexHullPrepareConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

        unsigned m_height = 0;

public:
        ConvexHullProgramPrepare(const vulkan::VulkanInstance& instance);

        ConvexHullProgramPrepare(const ConvexHullProgramPrepare&) = delete;
        ConvexHullProgramPrepare& operator=(const ConvexHullProgramPrepare&) = delete;
        ConvexHullProgramPrepare& operator=(ConvexHullProgramPrepare&&) = delete;

        ConvexHullProgramPrepare(ConvexHullProgramPrepare&&) = default;
        ~ConvexHullProgramPrepare() = default;

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
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

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
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

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer,
                            const vulkan::BufferWithMemory& points_buffer, const vulkan::BufferWithMemory& point_count_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
};
}
