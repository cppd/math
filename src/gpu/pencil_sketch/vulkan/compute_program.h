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

        PencilSketchComputeMemory m_memory;
        PencilSketchComputeConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

        unsigned m_groups_x = 0;
        unsigned m_groups_y = 0;

public:
        PencilSketchComputeProgram(const vulkan::VulkanInstance& instance);

        PencilSketchComputeProgram(const PencilSketchComputeProgram&) = delete;
        PencilSketchComputeProgram& operator=(const PencilSketchComputeProgram&) = delete;
        PencilSketchComputeProgram& operator=(PencilSketchComputeProgram&&) = delete;

        PencilSketchComputeProgram(PencilSketchComputeProgram&&) = default;
        ~PencilSketchComputeProgram() = default;

        void create_buffers(VkSampler sampler, const vulkan::ImageWithMemory& input_image,
                            const vulkan::ImageWithMemory& object_image, const vulkan::ImageWithMemory& output_image);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
};
}
