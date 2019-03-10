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

#include "vulkan_memory.h"

#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

namespace gpgpu_convex_hull_compute_vulkan_implementation
{
class ProgramPrepare final
{
        const vulkan::VulkanInstance& m_instance;

        PrepareMemory m_memory;
        PrepareConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

        unsigned m_height = 0;

public:
        ProgramPrepare(const vulkan::VulkanInstance& instance);

        ProgramPrepare(const ProgramPrepare&) = delete;
        ProgramPrepare& operator=(const ProgramPrepare&) = delete;
        ProgramPrepare& operator=(ProgramPrepare&&) = delete;

        ProgramPrepare(ProgramPrepare&&) = default;
        ~ProgramPrepare() = default;

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
};

class ProgramMerge final
{
        const vulkan::VulkanInstance& m_instance;

        MergeMemory m_memory;
        MergeConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

public:
        ProgramMerge(const vulkan::VulkanInstance& instance);

        ProgramMerge(const ProgramMerge&) = delete;
        ProgramMerge& operator=(const ProgramMerge&) = delete;
        ProgramMerge& operator=(ProgramMerge&&) = delete;

        ProgramMerge(ProgramMerge&&) = default;
        ~ProgramMerge() = default;

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
};

class ProgramFilter final
{
        const vulkan::VulkanInstance& m_instance;

        FilterMemory m_memory;
        FilterConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

public:
        ProgramFilter(const vulkan::VulkanInstance& instance);

        ProgramFilter(const ProgramFilter&) = delete;
        ProgramFilter& operator=(const ProgramFilter&) = delete;
        ProgramFilter& operator=(ProgramFilter&&) = delete;

        ProgramFilter(ProgramFilter&&) = default;
        ~ProgramFilter() = default;

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithMemory& lines_buffer,
                            const vulkan::BufferWithMemory& points_buffer, const vulkan::BufferWithMemory& point_count_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
};
}
