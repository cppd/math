/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "graphics.h"

#include "shader_info.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::vulkan::pipeline
{
namespace
{
VkPipelineVertexInputStateCreateInfo create_vertex_input_state_info(const GraphicsPipelineCreateInfo& info)
{
        VkPipelineVertexInputStateCreateInfo res = {};
        res.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        if (!info.binding_descriptions.empty())
        {
                res.vertexBindingDescriptionCount = info.binding_descriptions.size();
                res.pVertexBindingDescriptions = info.binding_descriptions.data();
        }

        if (!info.attribute_descriptions.empty())
        {
                res.vertexAttributeDescriptionCount = info.attribute_descriptions.size();
                res.pVertexAttributeDescriptions = info.attribute_descriptions.data();
        }

        return res;
}

VkPipelineInputAssemblyStateCreateInfo create_input_assembly_state_info(const GraphicsPipelineCreateInfo& info)
{
        const auto& primitive_topology = info.primitive_topology;
        ASSERT(primitive_topology);

        VkPipelineInputAssemblyStateCreateInfo res = {};
        res.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        res.topology = *primitive_topology;
        res.primitiveRestartEnable = VK_FALSE;
        return res;
}

VkViewport create_viewport(const GraphicsPipelineCreateInfo& info)
{
        const auto& viewport = info.viewport;
        ASSERT(viewport);
        ASSERT(viewport->is_positive());

        VkViewport res = {};
        res.x = viewport->x0();
        res.y = viewport->y0();
        res.width = viewport->width();
        res.height = viewport->height();
        res.minDepth = 0;
        res.maxDepth = 1;
        return res;
}

VkRect2D create_scissor(const GraphicsPipelineCreateInfo& info)
{
        const auto& viewport = info.viewport;
        ASSERT(viewport);

        VkRect2D res = {};
        res.offset.x = viewport->x0();
        res.offset.y = viewport->y0();
        res.extent.width = viewport->width();
        res.extent.height = viewport->height();
        return res;
}

VkPipelineViewportStateCreateInfo create_viewport_state_info(
        const VkViewport* const viewport,
        const VkRect2D* const scissor)
{
        VkPipelineViewportStateCreateInfo res = {};
        res.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        res.viewportCount = 1;
        res.pViewports = viewport;
        res.scissorCount = 1;
        res.pScissors = scissor;
        return res;
}

VkPipelineRasterizationStateCreateInfo create_rasterization_state_info(const GraphicsPipelineCreateInfo& info)
{
        VkPipelineRasterizationStateCreateInfo res = {};
        res.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        res.depthClampEnable = VK_FALSE;
        res.rasterizerDiscardEnable = VK_FALSE;
        res.polygonMode = VK_POLYGON_MODE_FILL;
        res.lineWidth = 1;
        res.cullMode = VK_CULL_MODE_NONE;
        res.frontFace = VK_FRONT_FACE_CLOCKWISE;
        res.depthBiasEnable = info.depth_bias ? VK_TRUE : VK_FALSE;
        // res.depthBiasConstantFactor = 0;
        // res.depthBiasClamp = 0;
        // res.depthBiasSlopeFactor = 0;
        return res;
}

VkPipelineMultisampleStateCreateInfo create_multisample_state_info(const GraphicsPipelineCreateInfo& info)
{
        const auto& sample_count = info.sample_count;
        ASSERT(sample_count);

        const auto& sample_shading = info.sample_shading;
        ASSERT(sample_shading);

        VkPipelineMultisampleStateCreateInfo res = {};
        res.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        res.rasterizationSamples = info.sample_count.value();
        if (*sample_count != VK_SAMPLE_COUNT_1_BIT && *sample_shading)
        {
                if (!info.device->features().features_10.sampleRateShading)
                {
                        error("Sample shading required but not supported");
                }
                res.sampleShadingEnable = VK_TRUE;
                res.minSampleShading = 1;
                LOG("Sample shading enabled");
        }
        else
        {
                res.sampleShadingEnable = VK_FALSE;
        }
        // res.pSampleMask = nullptr;
        // res.alphaToCoverageEnable = VK_FALSE;
        // res.alphaToOneEnable = VK_FALSE;
        return res;
}

std::vector<VkPipelineColorBlendAttachmentState> create_color_blend_attachment_states(
        const GraphicsPipelineCreateInfo& info)
{
        std::vector<VkPipelineColorBlendAttachmentState> res;

        if (info.color_blend.empty())
        {
                if (info.render_pass->color_attachment_count() > 0)
                {
                        VkPipelineColorBlendAttachmentState state = {};
                        state.colorWriteMask =
                                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                                | VK_COLOR_COMPONENT_A_BIT;
                        state.blendEnable = VK_FALSE;

                        res.resize(info.render_pass->color_attachment_count(), state);
                }
        }
        else
        {
                if (!(info.color_blend.size() == info.render_pass->color_attachment_count()))
                {
                        error("color blend count " + to_string(info.color_blend.size())
                              + " is not equal to color attachment count "
                              + to_string(info.render_pass->color_attachment_count()));
                }
                res = info.color_blend;
        }

        return res;
}

VkPipelineColorBlendStateCreateInfo create_color_blend_state_info(
        const uint32_t attachment_count,
        const VkPipelineColorBlendAttachmentState* const attachments)
{
        VkPipelineColorBlendStateCreateInfo res = {};
        res.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        res.logicOpEnable = VK_FALSE;
        // res.logicOp = VK_LOGIC_OP_COPY;
        res.attachmentCount = attachment_count;
        res.pAttachments = attachments;
        // res.blendConstants[0] = 0;
        // res.blendConstants[1] = 0;
        // res.blendConstants[2] = 0;
        // res.blendConstants[3] = 0;
        return res;
}

class DynamicStates final
{
        std::vector<VkDynamicState> dynamic_states_;
        VkPipelineDynamicStateCreateInfo state_info_;

public:
        explicit DynamicStates(const GraphicsPipelineCreateInfo& info)
        {
                // VK_DYNAMIC_STATE_VIEWPORT
                // VK_DYNAMIC_STATE_LINE_WIDTH
                if (info.depth_bias)
                {
                        dynamic_states_.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
                }

                state_info_ = {};
                state_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                state_info_.dynamicStateCount = dynamic_states_.size();
                state_info_.pDynamicStates = dynamic_states_.data();
        }

        DynamicStates(const DynamicStates&) = delete;
        DynamicStates& operator=(const DynamicStates&) = delete;

        [[nodiscard]] const VkPipelineDynamicStateCreateInfo* ptr() const
        {
                return state_info_.dynamicStateCount > 0 ? &state_info_ : nullptr;
        }
};

VkPipelineDepthStencilStateCreateInfo create_depth_stencil_state_info(const GraphicsPipelineCreateInfo& info)
{
        VkPipelineDepthStencilStateCreateInfo res = {};
        res.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        res.depthTestEnable = info.depth_test ? VK_TRUE : VK_FALSE;
        res.depthWriteEnable = info.depth_write ? VK_TRUE : VK_FALSE;
        res.depthCompareOp = VK_COMPARE_OP_LESS;
        res.depthBoundsTestEnable = VK_FALSE;
        // res.minDepthBounds = 0;
        // res.maxDepthBounds = 1;
        res.stencilTestEnable = VK_FALSE;
        // res.front = {};
        // res.back = {};
        return res;
}
}

handle::Pipeline create_graphics_pipeline(const GraphicsPipelineCreateInfo& info)
{
        if (!info.device || !info.render_pass || !info.sub_pass || !info.sample_count || !info.sample_shading
            || !info.pipeline_layout || !info.viewport || !info.primitive_topology || info.shaders.empty())
        {
                error("No required data to create graphics pipeline");
        }

        const PipelineShaderStageCreateInfo shader_stage_info(info.shaders, info.constants);

        const VkPipelineVertexInputStateCreateInfo vertex_input_state_info = create_vertex_input_state_info(info);

        const VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = create_input_assembly_state_info(info);

        const VkViewport viewport = create_viewport(info);

        const VkRect2D scissor = create_scissor(info);

        const VkPipelineViewportStateCreateInfo viewport_state_info = create_viewport_state_info(&viewport, &scissor);

        const VkPipelineRasterizationStateCreateInfo rasterization_state_info = create_rasterization_state_info(info);

        const VkPipelineMultisampleStateCreateInfo multisampling_state_info = create_multisample_state_info(info);

        const std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment_states =
                create_color_blend_attachment_states(info);

        const VkPipelineColorBlendStateCreateInfo color_blend_state_info = create_color_blend_state_info(
                color_blend_attachment_states.size(), color_blend_attachment_states.data());

        const DynamicStates dynamic_states(info);

        const VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = create_depth_stencil_state_info(info);

        const VkGraphicsPipelineCreateInfo create_info = [&]
        {
                VkGraphicsPipelineCreateInfo res = {};
                res.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                res.stageCount = shader_stage_info.size();
                res.pStages = shader_stage_info.data();
                res.pVertexInputState = &vertex_input_state_info;
                res.pInputAssemblyState = &input_assembly_state_info;
                res.pViewportState = &viewport_state_info;
                res.pRasterizationState = &rasterization_state_info;
                res.pMultisampleState = &multisampling_state_info;
                res.pDepthStencilState = &depth_stencil_state_info;
                res.pColorBlendState = &color_blend_state_info;
                res.pDynamicState = dynamic_states.ptr();
                res.layout = info.pipeline_layout.value();
                res.renderPass = info.render_pass->handle();
                res.subpass = info.sub_pass.value();
                // res.basePipelineHandle = VK_NULL_HANDLE;
                // res.basePipelineIndex = -1;
                return res;
        }();

        return {info.device->handle(), create_info};
}
}
