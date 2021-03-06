/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "pipeline.h"

#include <src/com/error.h>
#include <src/com/log.h>

#include <memory>

namespace ns::vulkan
{
namespace
{
void pipeline_shader_stage_create_info(
        const std::vector<const Shader*>& shaders,
        const std::vector<const SpecializationConstant*>& constants,
        std::vector<VkPipelineShaderStageCreateInfo>* create_info,
        std::vector<std::unique_ptr<VkSpecializationInfo>>* specialization_info)
{
        ASSERT(shaders.size() == constants.size());

        create_info->resize(shaders.size());
        specialization_info->clear();

        for (std::size_t i = 0; i < shaders.size(); ++i)
        {
                const Shader* shader = shaders[i];
                ASSERT(shader);

                (*create_info)[i] = {};
                (*create_info)[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                (*create_info)[i].stage = shader->stage();
                (*create_info)[i].module = shader->module();
                (*create_info)[i].pName = shader->entry_point_name();

                const SpecializationConstant* constant = constants[i];
                if (constant)
                {
                        specialization_info->push_back(std::make_unique<VkSpecializationInfo>());

                        *specialization_info->back() = {};
                        specialization_info->back()->mapEntryCount = constant->entries().size();
                        specialization_info->back()->pMapEntries = constant->entries().data();
                        specialization_info->back()->dataSize = constant->size();
                        specialization_info->back()->pData = constant->data();

                        (*create_info)[i].pSpecializationInfo = specialization_info->back().get();
                }
        }
}
}

Pipeline create_graphics_pipeline(const GraphicsPipelineCreateInfo& info)
{
        std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_info;
        std::vector<std::unique_ptr<VkSpecializationInfo>> specialization_info;
        pipeline_shader_stage_create_info(
                *info.shaders.value(), *info.constants.value(), &pipeline_shader_stage_info, &specialization_info);

        VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
        vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_info.vertexBindingDescriptionCount = info.binding_descriptions.value()->size();
        vertex_input_state_info.pVertexBindingDescriptions = info.binding_descriptions.value()->data();
        vertex_input_state_info.vertexAttributeDescriptionCount = info.attribute_descriptions.value()->size();
        vertex_input_state_info.pVertexAttributeDescriptions = info.attribute_descriptions.value()->data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {};
        input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_info.topology = info.primitive_topology.value();
        input_assembly_state_info.primitiveRestartEnable = VK_FALSE;

        ASSERT(info.viewport.value().is_positive());

        VkViewport viewport = {};
        viewport.x = info.viewport.value().x0();
        viewport.y = info.viewport.value().y0();
        viewport.width = info.viewport.value().width();
        viewport.height = info.viewport.value().height();
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset.x = info.viewport.value().x0();
        scissor.offset.y = info.viewport.value().y0();
        scissor.extent.width = info.viewport.value().width();
        scissor.extent.height = info.viewport.value().height();

        VkPipelineViewportStateCreateInfo viewport_state_info = {};
        viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_info.viewportCount = 1;
        viewport_state_info.pViewports = &viewport;
        viewport_state_info.scissorCount = 1;
        viewport_state_info.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterization_state_info = {};
        rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_info.depthClampEnable = VK_FALSE;

        rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;

        rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;

        rasterization_state_info.lineWidth = 1.0f;

        rasterization_state_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;

        rasterization_state_info.depthBiasEnable = info.depth_bias ? VK_TRUE : VK_FALSE;
        // rasterization_state_info.depthBiasConstantFactor = 0.0f;
        // rasterization_state_info.depthBiasClamp = 0.0f;
        // rasterization_state_info.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling_state_info = {};
        multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling_state_info.rasterizationSamples = info.sample_count.value();
        if (info.sample_count.value() != VK_SAMPLE_COUNT_1_BIT && info.sample_shading.value())
        {
                if (!info.device.value()->features().features_10.sampleRateShading)
                {
                        error("Sample shading required but not supported");
                }
                multisampling_state_info.sampleShadingEnable = VK_TRUE;
                multisampling_state_info.minSampleShading = 1.0f;
                LOG("Sample shading enabled");
        }
        else
        {
                multisampling_state_info.sampleShadingEnable = VK_FALSE;
        }
        // multisampling_state_info.pSampleMask = nullptr;
        // multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
        // multisampling_state_info.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
        if (!info.color_blend)
        {
                color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                                              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                color_blend_attachment_state.blendEnable = VK_FALSE;
        }
        else
        {
                color_blend_attachment_state = *info.color_blend;
        }

        VkPipelineColorBlendStateCreateInfo color_blending_state_info = {};
        color_blending_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending_state_info.logicOpEnable = VK_FALSE;
        // color_blending_state_info.logicOp = VK_LOGIC_OP_COPY;
        color_blending_state_info.attachmentCount = 1;
        color_blending_state_info.pAttachments = &color_blend_attachment_state;
        // color_blending_state_info.blendConstants[0] = 0.0f;
        // color_blending_state_info.blendConstants[1] = 0.0f;
        // color_blending_state_info.blendConstants[2] = 0.0f;
        // color_blending_state_info.blendConstants[3] = 0.0f;

        // VK_DYNAMIC_STATE_VIEWPORT
        // VK_DYNAMIC_STATE_LINE_WIDTH
        std::vector<VkDynamicState> dynamic_states;
        if (info.depth_bias)
        {
                dynamic_states.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
        }
        VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
        dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_info.dynamicStateCount = dynamic_states.size();
        dynamic_state_info.pDynamicStates = dynamic_states.data();

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {};
        depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state_info.depthTestEnable = info.depth_test ? VK_TRUE : VK_FALSE;
        depth_stencil_state_info.depthWriteEnable = info.depth_write ? VK_TRUE : VK_FALSE;

        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;

        depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
        // depth_stencil_state_info.minDepthBounds = 0.0f;
        // depth_stencil_state_info.maxDepthBounds = 1.0f;

        depth_stencil_state_info.stencilTestEnable = VK_FALSE;
        // depth_stencil_state_info.front = {};
        // depth_stencil_state_info.back = {};

        VkGraphicsPipelineCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = pipeline_shader_stage_info.size();
        create_info.pStages = pipeline_shader_stage_info.data();

        create_info.pVertexInputState = &vertex_input_state_info;
        create_info.pInputAssemblyState = &input_assembly_state_info;
        create_info.pViewportState = &viewport_state_info;
        create_info.pRasterizationState = &rasterization_state_info;
        create_info.pMultisampleState = &multisampling_state_info;
        create_info.pDepthStencilState = &depth_stencil_state_info;
        create_info.pColorBlendState = &color_blending_state_info;
        create_info.pDynamicState = dynamic_state_info.dynamicStateCount > 0 ? &dynamic_state_info : nullptr;

        create_info.layout = info.pipeline_layout.value();

        create_info.renderPass = info.render_pass.value();
        create_info.subpass = info.sub_pass.value();

        // create_info.basePipelineHandle = VK_NULL_HANDLE;
        // create_info.basePipelineIndex = -1;

        return Pipeline(*info.device.value(), create_info);
}

Pipeline create_compute_pipeline(const ComputePipelineCreateInfo& info)
{
        ASSERT(info.shader.value()->stage() == VK_SHADER_STAGE_COMPUTE_BIT);

        ASSERT(!info.constants.has_value() || info.constants.value() != nullptr);
        ASSERT(!info.constants.has_value() || info.constants.value()->size() > 0);
        ASSERT(!info.constants.has_value() || info.constants.value()->data() != nullptr);
        ASSERT(!info.constants.has_value() || !info.constants.value()->entries().empty());

        ASSERT(!info.constants.has_value()
               || std::all_of(
                       info.constants.value()->entries().cbegin(), info.constants.value()->entries().cend(),
                       [&](const VkSpecializationMapEntry& entry)
                       {
                               return entry.offset + entry.size <= info.constants.value()->size();
                       }));

        VkSpecializationInfo specialization_info;
        VkPipelineShaderStageCreateInfo stage_info = {};
        stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_info.stage = info.shader.value()->stage();
        stage_info.module = info.shader.value()->module();
        stage_info.pName = info.shader.value()->entry_point_name();
        if (info.constants.has_value())
        {
                specialization_info = {};
                specialization_info.mapEntryCount = info.constants.value()->entries().size();
                specialization_info.pMapEntries = info.constants.value()->entries().data();
                specialization_info.dataSize = info.constants.value()->size();
                specialization_info.pData = info.constants.value()->data();
                stage_info.pSpecializationInfo = &specialization_info;
        }

        VkComputePipelineCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        create_info.stage = stage_info;
        create_info.layout = info.pipeline_layout.value();

        return Pipeline(*info.device.value(), create_info);
}
}
