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

#include "constant.h"
#include "objects.h"
#include "shader.h"

#include <optional>
#include <vector>

namespace vulkan
{
struct GraphicsPipelineCreateInfo
{
        std::optional<const Device*> device;
        std::optional<VkRenderPass> render_pass;
        std::optional<uint32_t> sub_pass;
        std::optional<VkSampleCountFlagBits> sample_count;
        std::optional<bool> sample_shading;
        std::optional<VkPipelineLayout> pipeline_layout;
        std::optional<uint32_t> viewport_x;
        std::optional<uint32_t> viewport_y;
        std::optional<uint32_t> viewport_width;
        std::optional<uint32_t> viewport_height;
        std::optional<VkPrimitiveTopology> primitive_topology;
        std::optional<const std::vector<const Shader*>*> shaders;
        std::optional<const std::vector<const SpecializationConstant*>*> constants;
        std::optional<const std::vector<VkVertexInputBindingDescription>*> binding_descriptions;
        std::optional<const std::vector<VkVertexInputAttributeDescription>*> attribute_descriptions;
        std::optional<bool> depth_bias;
        std::optional<bool> color_blend;
};

Pipeline create_graphics_pipeline(const GraphicsPipelineCreateInfo& info);

struct ComputePipelineCreateInfo
{
        std::optional<const Device*> device;
        std::optional<VkPipelineLayout> pipeline_layout;
        std::optional<const ComputeShader*> shader;
        std::optional<const SpecializationConstant*> constants;
};

Pipeline create_compute_pipeline(const ComputePipelineCreateInfo& info);
}
