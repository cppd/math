/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "device.h"
#include "objects.h"
#include "shader.h"

#include <src/numerical/region.h>

#include <optional>
#include <vector>

namespace ns::vulkan
{
struct GraphicsPipelineCreateInfo final
{
        // required, std::optional is used to check that the values are set
        const Device* device = nullptr;
        const RenderPass* render_pass = nullptr;
        std::optional<std::uint32_t> sub_pass;
        std::optional<VkSampleCountFlagBits> sample_count;
        std::optional<bool> sample_shading;
        std::optional<VkPipelineLayout> pipeline_layout;
        std::optional<Region<2, int>> viewport;
        std::optional<VkPrimitiveTopology> primitive_topology;
        std::vector<const Shader*> shaders;

        bool depth_bias = false;
        bool depth_test = true;
        bool depth_write = true;

        // optional
        const std::vector<VkVertexInputBindingDescription>* binding_descriptions = nullptr;
        const std::vector<VkVertexInputAttributeDescription>* attribute_descriptions = nullptr;
        std::vector<VkSpecializationInfo> constants;
        std::vector<VkPipelineColorBlendAttachmentState> color_blend;
};

handle::Pipeline create_graphics_pipeline(const GraphicsPipelineCreateInfo& info);

//

struct ComputePipelineCreateInfo final
{
        // required
        VkDevice device = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        const Shader* shader = nullptr;

        // optional
        const VkSpecializationInfo* constants = nullptr;
};

handle::Pipeline create_compute_pipeline(const ComputePipelineCreateInfo& info);

//

struct RayTracingPipelineCreateInfo final
{
        // required
        VkDevice device = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        std::vector<const Shader*> shaders;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups;

        // optional
        std::vector<VkSpecializationInfo> constants;
};

handle::Pipeline create_ray_tracing_pipeline(const RayTracingPipelineCreateInfo& info);
}
