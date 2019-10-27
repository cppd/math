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

namespace gpu_vulkan
{
class PencilSketchComputeProgram final
{
        const vulkan::VulkanInstance& m_instance;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        PencilSketchComputeConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        PencilSketchComputeProgram(const vulkan::VulkanInstance& instance);

        PencilSketchComputeProgram(const PencilSketchComputeProgram&) = delete;
        PencilSketchComputeProgram& operator=(const PencilSketchComputeProgram&) = delete;
        PencilSketchComputeProgram& operator=(PencilSketchComputeProgram&&) = delete;

        PencilSketchComputeProgram(PencilSketchComputeProgram&&) = default;
        ~PencilSketchComputeProgram() = default;

        void create_pipeline(unsigned group_size, unsigned x, unsigned y, unsigned width, unsigned height);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
