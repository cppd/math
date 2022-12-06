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

#include "graphics.h"

#include "shader_info.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>

namespace ns::vulkan
{
namespace
{
VkPipelineVertexInputStateCreateInfo create_vertex_input_state_info(const GraphicsPipelineCreateInfo& info)
{
        VkPipelineVertexInputStateCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        if (!info.binding_descriptions.empty())
        {
                create_info.vertexBindingDescriptionCount = info.binding_descriptions.size();
                create_info.pVertexBindingDescriptions = info.binding_descriptions.data();
        }

        if (!info.attribute_descriptions.empty())
        {
                create_info.vertexAttributeDescriptionCount = info.attribute_descriptions.size();
                create_info.pVertexAttributeDescriptions = info.attribute_descriptions.data();
        }

        return create_info;
}

VkPipelineInputAssemblyStateCreateInfo create_input_assembly_state_info(const GraphicsPipelineCreateInfo& info)
{
        VkPipelineInputAssemblyStateCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        create_info.topology = info.primitive_topology.value();
        create_info.primitiveRestartEnable = VK_FALSE;
        return create_info;
}

VkViewport create_viewport(const GraphicsPipelineCreateInfo& info)
{
        ASSERT(info.viewport.value().is_positive());

        VkViewport viewport = {};
        viewport.x = info.viewport.value().x0();
        viewport.y = info.viewport.value().y0();
        viewport.width = info.viewport.value().width();
        viewport.height = info.viewport.value().height();
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        return viewport;
}

VkRect2D create_scissor(const GraphicsPipelineCreateInfo& info)
{
        VkRect2D scissor = {};
        scissor.offset.x = info.viewport.value().x0();
        scissor.offset.y = info.viewport.value().y0();
        scissor.extent.width = info.viewport.value().width();
        scissor.extent.height = info.viewport.value().height();
        return scissor;
}

VkPipelineViewportStateCreateInfo create_viewport_state_info(
        const VkViewport* const viewport,
        const VkRect2D* const scissor)
{
        VkPipelineViewportStateCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        create_info.viewportCount = 1;
        create_info.pViewports = viewport;
        create_info.scissorCount = 1;
        create_info.pScissors = scissor;
        return create_info;
}

VkPipelineRasterizationStateCreateInfo create_rasterization_state_info(const GraphicsPipelineCreateInfo& info)
{
        VkPipelineRasterizationStateCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        create_info.depthClampEnable = VK_FALSE;
        create_info.rasterizerDiscardEnable = VK_FALSE;
        create_info.polygonMode = VK_POLYGON_MODE_FILL;
        create_info.lineWidth = 1.0f;
        create_info.cullMode = VK_CULL_MODE_NONE;
        create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        create_info.depthBiasEnable = info.depth_bias ? VK_TRUE : VK_FALSE;
        // create_info.depthBiasConstantFactor = 0.0f;
        // create_info.depthBiasClamp = 0.0f;
        // create_info.depthBiasSlopeFactor = 0.0f;
        return create_info;
}

VkPipelineMultisampleStateCreateInfo create_multisample_state_info(const GraphicsPipelineCreateInfo& info)
{
        VkPipelineMultisampleStateCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        create_info.rasterizationSamples = info.sample_count.value();
        if (info.sample_count.value() != VK_SAMPLE_COUNT_1_BIT && info.sample_shading.value())
        {
                if (!info.device->features().features_10.sampleRateShading)
                {
                        error("Sample shading required but not supported");
                }
                create_info.sampleShadingEnable = VK_TRUE;
                create_info.minSampleShading = 1.0f;
                LOG("Sample shading enabled");
        }
        else
        {
                create_info.sampleShadingEnable = VK_FALSE;
        }
        // create_info.pSampleMask = nullptr;
        // create_info.alphaToCoverageEnable = VK_FALSE;
        // create_info.alphaToOneEnable = VK_FALSE;
        return create_info;
}

std::vector<VkPipelineColorBlendAttachmentState> create_color_blend_attachment_states(
        const GraphicsPipelineCreateInfo& info)
{
        std::vector<VkPipelineColorBlendAttachmentState> states;

        if (info.color_blend.empty())
        {
                if (info.render_pass->color_attachment_count() > 0)
                {
                        VkPipelineColorBlendAttachmentState state = {};
                        state.colorWriteMask =
                                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                                | VK_COLOR_COMPONENT_A_BIT;
                        state.blendEnable = VK_FALSE;

                        states.resize(info.render_pass->color_attachment_count(), state);
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
                states = info.color_blend;
        }

        return states;
}

VkPipelineColorBlendStateCreateInfo create_color_blend_state_info(
        const uint32_t attachment_count,
        const VkPipelineColorBlendAttachmentState* const attachments)
{
        VkPipelineColorBlendStateCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        create_info.logicOpEnable = VK_FALSE;
        // create_info.logicOp = VK_LOGIC_OP_COPY;
        create_info.attachmentCount = attachment_count;
        create_info.pAttachments = attachments;
        // create_info.blendConstants[0] = 0.0f;
        // create_info.blendConstants[1] = 0.0f;
        // create_info.blendConstants[2] = 0.0f;
        // create_info.blendConstants[3] = 0.0f;
        return create_info;
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
        VkPipelineDepthStencilStateCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        create_info.depthTestEnable = info.depth_test ? VK_TRUE : VK_FALSE;
        create_info.depthWriteEnable = info.depth_write ? VK_TRUE : VK_FALSE;
        create_info.depthCompareOp = VK_COMPARE_OP_LESS;
        create_info.depthBoundsTestEnable = VK_FALSE;
        // create_info.minDepthBounds = 0.0f;
        // create_info.maxDepthBounds = 1.0f;
        create_info.stencilTestEnable = VK_FALSE;
        // create_info.front = {};
        // create_info.back = {};
        return create_info;
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
