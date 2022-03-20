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

#include "program_fragments.h"

#include "descriptors.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> FragmentsProgram::descriptor_set_layout_shared_bindings()
{
        return VolumeSharedMemory::descriptor_set_layout_bindings(0, 0, 0, 0, 0);
}

FragmentsProgram::FragmentsProgram(const vulkan::Device* const device, const Code& code)
        : device_(device),
          descriptor_set_layout_shared_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_shared_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(
                  *device,
                  {VolumeSharedMemory::set_number()},
                  {descriptor_set_layout_shared_})),
          vertex_shader_(*device_, code.volume_vert(), VK_SHADER_STAGE_VERTEX_BIT),
          fragment_shader_(*device_, code.volume_fragments_frag(), VK_SHADER_STAGE_FRAGMENT_BIT)
{
}

VkDescriptorSetLayout FragmentsProgram::descriptor_set_layout_shared() const
{
        return descriptor_set_layout_shared_;
}

VkPipelineLayout FragmentsProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

vulkan::handle::Pipeline FragmentsProgram::create_pipeline(
        const VkRenderPass render_pass,
        const VkSampleCountFlagBits sample_count,
        const bool sample_shading,
        const Region<2, int>& viewport) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_test = false;
        info.depth_write = false;

        info.color_blend.emplace();
        info.color_blend->colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                | VK_COLOR_COMPONENT_A_BIT;
        info.color_blend->blendEnable = VK_TRUE;
        info.color_blend->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        info.color_blend->dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        info.color_blend->colorBlendOp = VK_BLEND_OP_ADD;
        info.color_blend->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        info.color_blend->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        info.color_blend->alphaBlendOp = VK_BLEND_OP_ADD;

        const std::vector<const vulkan::Shader*> shaders{&vertex_shader_, &fragment_shader_};
        const std::vector<const vulkan::SpecializationConstant*> specialization_constants = {nullptr, nullptr};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions;
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        info.shaders = &shaders;
        info.constants = &specialization_constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
