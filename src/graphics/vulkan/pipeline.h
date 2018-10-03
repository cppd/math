/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "objects.h"
#include "shader.h"

#include <vector>

namespace vulkan
{
PipelineLayout create_pipeline_layout(VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);

Pipeline create_graphics_pipeline(const Device& device, VkRenderPass render_pass, uint32_t sub_pass,
                                  VkSampleCountFlagBits sample_count, VkPipelineLayout pipeline_layout, uint32_t width,
                                  uint32_t height, VkPrimitiveTopology primitive_topology,
                                  const std::vector<const Shader*>& shaders,
                                  const std::vector<VkVertexInputBindingDescription>& binding_descriptions,
                                  const std::vector<VkVertexInputAttributeDescription>& attribute_descriptions);

Pipeline create_shadow_graphics_pipeline(const Device& device, VkRenderPass render_pass, uint32_t sub_pass,
                                         VkSampleCountFlagBits sample_count, VkPipelineLayout pipeline_layout, uint32_t width,
                                         uint32_t height, VkPrimitiveTopology primitive_topology,
                                         const std::vector<const Shader*>& shaders,
                                         const std::vector<VkVertexInputBindingDescription>& binding_descriptions,
                                         const std::vector<VkVertexInputAttributeDescription>& attribute_descriptions);
}
