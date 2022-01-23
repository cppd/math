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

#include "constant.h"
#include "device.h"
#include "objects.h"
#include "shader.h"

#include <src/numerical/region.h>

#include <optional>
#include <vector>

namespace ns::vulkan
{
struct GraphicsPipelineCreateInfo
{
        // required, std::optional is used to check that values are set
        const Device* device = nullptr;
        std::optional<VkRenderPass> render_pass;
        std::optional<std::uint32_t> sub_pass;
        std::optional<VkSampleCountFlagBits> sample_count;
        std::optional<bool> sample_shading;
        std::optional<VkPipelineLayout> pipeline_layout;
        std::optional<Region<2, int>> viewport;
        std::optional<VkPrimitiveTopology> primitive_topology;
        const std::vector<const Shader*>* shaders = nullptr;
        const std::vector<const SpecializationConstant*>* constants = nullptr;
        const std::vector<VkVertexInputBindingDescription>* binding_descriptions = nullptr;
        const std::vector<VkVertexInputAttributeDescription>* attribute_descriptions = nullptr;

        bool depth_bias = false;
        bool depth_test = true;
        bool depth_write = true;

        // optional
        std::optional<VkPipelineColorBlendAttachmentState> color_blend;
};

handle::Pipeline create_graphics_pipeline(const GraphicsPipelineCreateInfo& info);

//

struct ComputePipelineCreateInfo
{
        // required
        VkDevice device = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        const Shader* shader = nullptr;

        // optional
        const SpecializationConstant* constants = nullptr;
};

handle::Pipeline create_compute_pipeline(const ComputePipelineCreateInfo& info);

//

struct RayTracingPipelineCreateInfo
{
        // required
        VkDevice device = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        const std::vector<const Shader*>* shaders = nullptr;
        const std::vector<VkRayTracingShaderGroupCreateInfoKHR>* shader_groups = nullptr;

        // optional
        const std::vector<const SpecializationConstant*>* constants = nullptr;
};

handle::Pipeline create_ray_tracing_pipeline(const RayTracingPipelineCreateInfo& info);
}
