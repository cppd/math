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

#include "com/color/color.h"
#include "graphics/vulkan/constant.h"
#include "graphics/vulkan/shader.h"

#include <functional>
#include <optional>
#include <vector>

namespace gpu_vulkan
{
class RenderBuffers3D
{
protected:
        virtual ~RenderBuffers3D() = default;

public:
        virtual vulkan::CommandBuffers create_command_buffers(
                const Color& clear_color,
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) = 0;

        virtual VkRenderPass render_pass() const = 0;
        virtual VkSampleCountFlagBits sample_count() const = 0;
};

class RenderBuffers2D
{
protected:
        virtual ~RenderBuffers2D() = default;

public:
        virtual std::vector<VkCommandBuffer> create_command_buffers(
                const std::optional<std::function<void(VkCommandBuffer buffer)>>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer buffer)>& commands) = 0;

        virtual void delete_command_buffers(std::vector<VkCommandBuffer>* buffers) = 0;

        virtual VkPipeline create_pipeline(VkPrimitiveTopology primitive_topology, bool sample_shading, bool color_blend,
                                           const std::vector<const vulkan::Shader*>& shaders,
                                           const std::vector<const vulkan::SpecializationConstant*>& constants,
                                           VkPipelineLayout pipeline_layout,
                                           const std::vector<VkVertexInputBindingDescription>& vertex_binding,
                                           const std::vector<VkVertexInputAttributeDescription>& vertex_attribute, unsigned x,
                                           unsigned y, unsigned width, unsigned height) = 0;
};
}
