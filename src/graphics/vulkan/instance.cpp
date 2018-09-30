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

#include "instance.h"

#include "common.h"
#include "debug.h"
#include "query.h"

#include "application/application_name.h"
#include "com/alg.h"
#include "com/color/conversion.h"
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"

namespace
{
VkClearValue color_float_srgb_clear_value(const Color& clear_color)
{
        VkClearValue clear_value;
        clear_value.color.float32[0] = color_conversion::rgb_float_to_srgb_float(clear_color.red());
        clear_value.color.float32[1] = color_conversion::rgb_float_to_srgb_float(clear_color.green());
        clear_value.color.float32[2] = color_conversion::rgb_float_to_srgb_float(clear_color.blue());
        clear_value.color.float32[3] = 1;
        return clear_value;
}
VkClearValue depth_stencil_clear_value()
{
        VkClearValue clear_value;
        clear_value.depthStencil.depth = 1;
        clear_value.depthStencil.stencil = 0;
        return clear_value;
}

std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_info(const std::vector<const vulkan::Shader*>& shaders)
{
        std::vector<VkPipelineShaderStageCreateInfo> res;

        for (const vulkan::Shader* s : shaders)
        {
                VkPipelineShaderStageCreateInfo stage_info = {};
                stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stage_info.stage = s->stage();
                stage_info.module = s->module();
                stage_info.pName = s->entry_point_name();

                res.push_back(stage_info);
        }

        return res;
}

vulkan::Instance create_instance(int api_version_major, int api_version_minor, std::vector<std::string> required_extensions,
                                 const std::vector<std::string>& required_validation_layers)
{
        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        if (required_validation_layers.size() > 0)
        {
                required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        vulkan::check_api_version(required_api_version);
        vulkan::check_instance_extension_support(required_extensions);
        vulkan::check_validation_layer_support(required_validation_layers);

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = APPLICATION_NAME;
        app_info.applicationVersion = 1;
        app_info.pEngineName = nullptr;
        app_info.engineVersion = 0;
        app_info.apiVersion = required_api_version;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        const std::vector<const char*> extensions = const_char_pointer_vector(required_extensions);
        if (extensions.size() > 0)
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        const std::vector<const char*> validation_layers = const_char_pointer_vector(required_validation_layers);
        if (validation_layers.size() > 0)
        {
                create_info.enabledLayerCount = validation_layers.size();
                create_info.ppEnabledLayerNames = validation_layers.data();
        }

        return vulkan::Instance(create_info);
}

vulkan::RenderPass create_render_pass(VkDevice device, VkFormat swapchain_image_format, VkFormat depth_image_format)
{
        std::array<VkAttachmentDescription, 2> attachments = {};

        // Color
        attachments[0].format = swapchain_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Depth
        attachments[1].format = depth_image_format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_reference = {};
        color_reference.attachment = 0;
        color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 1;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_reference;
        subpass_description.pDepthStencilAttachment = &depth_reference;

#if 1
        std::array<VkSubpassDependency, 1> subpass_dependencies = {};
        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
#else
        std::array<VkSubpassDependency, 2> subpass_dependencies = {};

        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpass_dependencies[1].srcSubpass = 0;
        subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[1].dstAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
#endif

        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_description;
        create_info.dependencyCount = subpass_dependencies.size();
        create_info.pDependencies = subpass_dependencies.data();

        return vulkan::RenderPass(device, create_info);
}

vulkan::RenderPass create_multisampling_render_pass(VkDevice device, VkSampleCountFlagBits sample_count,
                                                    VkFormat swapchain_image_format, VkFormat depth_image_format)
{
        std::array<VkAttachmentDescription, 3> attachments = {};

        // Color resolve
        attachments[0].format = swapchain_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Multisampling color
        attachments[1].format = swapchain_image_format;
        attachments[1].samples = sample_count;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Multisampling depth
        attachments[2].format = depth_image_format;
        attachments[2].samples = sample_count;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference multisampling_color_reference = {};
        multisampling_color_reference.attachment = 1;
        multisampling_color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference multisampling_depth_reference = {};
        multisampling_depth_reference.attachment = 2;
        multisampling_depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_resolve_reference = {};
        color_resolve_reference.attachment = 0;
        color_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &multisampling_color_reference;
        subpass_description.pResolveAttachments = &color_resolve_reference;
        subpass_description.pDepthStencilAttachment = &multisampling_depth_reference;

#if 1
        std::array<VkSubpassDependency, 1> subpass_dependencies = {};
        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
#else
        std::array<VkSubpassDependency, 2> subpass_dependencies = {};

        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[0].srcAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpass_dependencies[1].srcSubpass = 0;
        subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[1].dstAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
#endif

        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_description;
        create_info.dependencyCount = subpass_dependencies.size();
        create_info.pDependencies = subpass_dependencies.data();

        return vulkan::RenderPass(device, create_info);
}

vulkan::RenderPass create_shadow_render_pass[[maybe_unused]](VkDevice device, VkFormat depth_image_format)
{
        std::array<VkAttachmentDescription, 1> attachments = {};

        // Depth
        attachments[0].format = depth_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 0;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 0;
        subpass_description.pDepthStencilAttachment = &depth_reference;

        std::array<VkSubpassDependency, 2> subpass_dependencies = {};

        subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[0].dstSubpass = 0;
        subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpass_dependencies[0].srcAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[0].dstAccessMask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpass_dependencies[1].srcSubpass = 0;
        subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        subpass_dependencies[1].srcAccessMask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpass_dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // VK_ACCESS_MEMORY_READ_BIT;
        subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_description;
        create_info.dependencyCount = subpass_dependencies.size();
        create_info.pDependencies = subpass_dependencies.data();

        return vulkan::RenderPass(device, create_info);
}

template <size_t N>
vulkan::Framebuffer create_framebuffer(VkDevice device, VkRenderPass render_pass, uint32_t width, uint32_t height,
                                       const std::array<VkImageView, N>& attachments)
{
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.width = width;
        create_info.height = height;
        create_info.layers = 1;

        return vulkan::Framebuffer(device, create_info);
}

vulkan::PipelineLayout create_pipeline_layout(VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
{
        VkPipelineLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = descriptor_set_layouts.size();
        create_info.pSetLayouts = descriptor_set_layouts.data();
        // create_info.pushConstantRangeCount = 0;
        // create_info.pPushConstantRanges = nullptr;

        return vulkan::PipelineLayout(device, create_info);
}

vulkan::Pipeline create_graphics_pipeline(const vulkan::Device& device, VkRenderPass render_pass, uint32_t sub_pass,
                                          VkSampleCountFlagBits sample_count, VkPipelineLayout pipeline_layout, uint32_t width,
                                          uint32_t height, const std::vector<const vulkan::Shader*>& shaders,
                                          const std::vector<VkVertexInputBindingDescription>& binding_descriptions,
                                          const std::vector<VkVertexInputAttributeDescription>& attribute_descriptions,
                                          bool for_shadow)
{
        std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stages = pipeline_shader_stage_create_info(shaders);

        VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
        vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_info.vertexBindingDescriptionCount = binding_descriptions.size();
        vertex_input_state_info.pVertexBindingDescriptions = binding_descriptions.data();
        vertex_input_state_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
        vertex_input_state_info.pVertexAttributeDescriptions = attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {};
        input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_state_info.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent.width = width;
        scissor.extent.height = height;

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

        if (for_shadow)
        {
                rasterization_state_info.depthBiasEnable = VK_TRUE;
        }
        else
        {
                rasterization_state_info.depthBiasEnable = VK_FALSE;
        }
        // rasterization_state_info.depthBiasConstantFactor = 0.0f;
        // rasterization_state_info.depthBiasClamp = 0.0f;
        // rasterization_state_info.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling_state_info = {};
        multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling_state_info.rasterizationSamples = sample_count;
        multisampling_state_info.sampleShadingEnable = device.features().sampleRateShading;
        multisampling_state_info.minSampleShading = 1.0f;
        // multisampling_state_info.pSampleMask = nullptr;
        // multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
        // multisampling_state_info.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
        color_blend_attachment_state.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (true)
        {
                color_blend_attachment_state.blendEnable = VK_FALSE;
                // color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                // color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                // color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
                // color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                // color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                // color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        else
        {
                color_blend_attachment_state.blendEnable = VK_TRUE;
                color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
                color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
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
        std::vector<VkDynamicState> dynamic_states({VK_DYNAMIC_STATE_DEPTH_BIAS});
        VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
        dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_info.dynamicStateCount = dynamic_states.size();
        dynamic_state_info.pDynamicStates = dynamic_states.data();

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {};
        depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state_info.depthTestEnable = VK_TRUE;
        depth_stencil_state_info.depthWriteEnable = VK_TRUE;

        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;

        depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
        // depth_stencil_state_info.minDepthBounds = 0.0f;
        // depth_stencil_state_info.maxDepthBounds = 1.0f;

        depth_stencil_state_info.stencilTestEnable = VK_FALSE;
        // depth_stencil_state_info.front = {};
        // depth_stencil_state_info.back = {};

        VkGraphicsPipelineCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = pipeline_shader_stages.size();
        create_info.pStages = pipeline_shader_stages.data();

        create_info.pVertexInputState = &vertex_input_state_info;
        create_info.pInputAssemblyState = &input_assembly_state_info;
        create_info.pViewportState = &viewport_state_info;
        create_info.pRasterizationState = &rasterization_state_info;
        create_info.pMultisampleState = &multisampling_state_info;
        create_info.pDepthStencilState = &depth_stencil_state_info;
        create_info.pColorBlendState = &color_blending_state_info;
        create_info.pDynamicState = for_shadow ? &dynamic_state_info : nullptr;

        create_info.layout = pipeline_layout;

        create_info.renderPass = render_pass;
        create_info.subpass = sub_pass;

        // create_info.basePipelineHandle = VK_NULL_HANDLE;
        // create_info.basePipelineIndex = -1;

        return vulkan::Pipeline(device, create_info);
}

vulkan::CommandPool create_command_pool(VkDevice device, uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;

        return vulkan::CommandPool(device, create_info);
}

vulkan::CommandPool create_transient_command_pool(VkDevice device, uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;
        create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        return vulkan::CommandPool(device, create_info);
}

template <size_t N>
vulkan::CommandBuffers create_command_buffers(
        VkDevice device, uint32_t width, uint32_t height, VkRenderPass render_pass, VkPipelineLayout pipeline_layout,
        VkPipeline pipeline, const std::vector<vulkan::Framebuffer>& framebuffers, VkCommandPool command_pool,
        const std::array<VkClearValue, N>& clear_values,
        const std::function<void(VkPipelineLayout pipeline_layout, VkPipeline pipeline, VkCommandBuffer command_buffer)>&
                commands_for_triangle_topology)
{
        VkResult result;

        vulkan::CommandBuffers command_buffers(device, command_pool, framebuffers.size());

        for (uint32_t i = 0; i < command_buffers.count(); ++i)
        {
                VkCommandBufferBeginInfo command_buffer_info = {};
                command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                // command_buffer_info.pInheritanceInfo = nullptr;

                result = vkBeginCommandBuffer(command_buffers[i], &command_buffer_info);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                }

                VkRenderPassBeginInfo render_pass_info = {};
                render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_info.renderPass = render_pass;
                render_pass_info.framebuffer = framebuffers[i];
                render_pass_info.renderArea.offset = {0, 0};
                render_pass_info.renderArea.extent.width = width;
                render_pass_info.renderArea.extent.height = height;
                render_pass_info.clearValueCount = clear_values.size();
                render_pass_info.pClearValues = clear_values.data();

                vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

#if 1
                commands_for_triangle_topology(pipeline_layout, pipeline, command_buffers[i]);

#else
                vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                                        &descriptor_set, 0, nullptr);

                VkBuffer vertex_buffers[] = {vertex_buffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);

#if 0
                vkCmdDraw(command_buffers[i], vertex_count, 1, 0, 0);
#else
                vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, index_type);
                vkCmdDrawIndexed(command_buffers[i], vertex_count, 1, 0, 0, 0);
#endif
#endif

                vkCmdEndRenderPass(command_buffers[i]);

                result = vkEndCommandBuffer(command_buffers[i]);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                }
        }

        return command_buffers;
}

std::vector<vulkan::Semaphore> create_semaphores(VkDevice device, int count)
{
        std::vector<vulkan::Semaphore> res;
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device);
        }
        return res;
}

std::vector<vulkan::Fence> create_fences(VkDevice device, int count, bool signaled_state)
{
        std::vector<vulkan::Fence> res;
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device, signaled_state);
        }
        return res;
}
}

namespace vulkan
{
SwapchainAndBuffers::SwapchainAndBuffers(VkSurfaceKHR surface, VkPhysicalDevice physical_device,
                                         const std::vector<uint32_t>& swapchain_family_indices,
                                         const std::vector<uint32_t>& attachment_family_indices, const Device& device,
                                         VkCommandPool graphics_command_pool, VkQueue graphics_queue, int preferred_image_count,
                                         int required_minimum_sample_count, const std::vector<const vulkan::Shader*>& shaders,
                                         const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                         const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
                                         const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts,
                                         const std::vector<const vulkan::Shader*>& shadow_shaders,
                                         const std::vector<VkDescriptorSetLayout>& shadow_descriptor_set_layouts,
                                         double shadow_zoom)
        : m_device(device),
          m_graphics_command_pool(graphics_command_pool),
          m_sample_count_bit(supported_framebuffer_sample_count_flag(physical_device, required_minimum_sample_count))
{
        ASSERT(surface != VK_NULL_HANDLE);
        ASSERT(physical_device != VK_NULL_HANDLE);
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(graphics_command_pool != VK_NULL_HANDLE);
        ASSERT(graphics_queue != VK_NULL_HANDLE);

        ASSERT(descriptor_set_layouts.size() > 0);
        ASSERT(swapchain_family_indices.size() > 0);
        ASSERT(attachment_family_indices.size() > 0);

        LOG("Minimum sample count = " + to_string(required_minimum_sample_count) +
            ", chosen sample count = " + to_string(integer_sample_count_flag(m_sample_count_bit)));

        m_swapchain = std::make_unique<Swapchain>(surface, device, swapchain_family_indices, preferred_image_count);

        if (m_sample_count_bit != VK_SAMPLE_COUNT_1_BIT)
        {
                m_multisampling_color_attachment = std::make_unique<ColorAttachment>(
                        device, graphics_command_pool, graphics_queue, attachment_family_indices, m_swapchain->format(),
                        m_sample_count_bit, m_swapchain->width(), m_swapchain->height());

                m_multisampling_depth_attachment = std::make_unique<DepthAttachment>(
                        device, graphics_command_pool, graphics_queue, attachment_family_indices, m_sample_count_bit,
                        m_swapchain->width(), m_swapchain->height());

                m_render_pass = create_multisampling_render_pass(device, m_sample_count_bit, m_swapchain->format(),
                                                                 m_multisampling_depth_attachment->format());

                for (VkImageView swapchain_image_view : m_swapchain->image_views())
                {
                        std::array<VkImageView, 3> attachments;
                        attachments[0] = swapchain_image_view;
                        attachments[1] = m_multisampling_color_attachment->image_view();
                        attachments[2] = m_multisampling_depth_attachment->image_view();

                        m_framebuffers.push_back(create_framebuffer(device, m_render_pass, m_swapchain->width(),
                                                                    m_swapchain->height(), attachments));
                }

                LOG("Multisampling color attachment format " + format_to_string(m_multisampling_color_attachment->format()));
                LOG("Multisampling depth attachment format " + format_to_string(m_multisampling_depth_attachment->format()));
        }
        else
        {
                m_depth_attachment = std::make_unique<DepthAttachment>(device, graphics_command_pool, graphics_queue,
                                                                       attachment_family_indices, VK_SAMPLE_COUNT_1_BIT,
                                                                       m_swapchain->width(), m_swapchain->height());

                m_render_pass = create_render_pass(device, m_swapchain->format(), m_depth_attachment->format());

                for (VkImageView swapchain_image_view : m_swapchain->image_views())
                {
                        std::array<VkImageView, 2> attachments;
                        attachments[0] = swapchain_image_view;
                        attachments[1] = m_depth_attachment->image_view();

                        m_framebuffers.push_back(create_framebuffer(device, m_render_pass, m_swapchain->width(),
                                                                    m_swapchain->height(), attachments));
                }

                LOG("Depth attachment format " + format_to_string(m_depth_attachment->format()));
        }

        m_pipeline_layout = create_pipeline_layout(device, descriptor_set_layouts);
        m_pipeline = create_graphics_pipeline(device, m_render_pass, 0 /*sub_pass*/, m_sample_count_bit, m_pipeline_layout,
                                              m_swapchain->width(), m_swapchain->height(), shaders, vertex_binding_descriptions,
                                              vertex_attribute_descriptions, false /*for_shadow*/);

        //

        m_shadow_width = std::round(m_swapchain->width() * shadow_zoom);
        m_shadow_height = std::round(m_swapchain->height() * shadow_zoom);

        uint32_t old_shadow_width = m_shadow_width;
        uint32_t old_shadow_height = m_shadow_height;
        m_shadow_depth_attachment = std::make_unique<ShadowDepthAttachment>(
                device, graphics_command_pool, graphics_queue, attachment_family_indices, &m_shadow_width, &m_shadow_height);

        m_shadow_render_pass = create_shadow_render_pass(device, m_shadow_depth_attachment->format());
        std::array<VkImageView, 1> shadow_attachments = {m_shadow_depth_attachment->image_view()};
        m_shadow_framebuffers.push_back(
                create_framebuffer(device, m_shadow_render_pass, m_shadow_width, m_shadow_height, shadow_attachments));
        m_shadow_pipeline_layout = create_pipeline_layout(device, shadow_descriptor_set_layouts);
        m_shadow_pipeline = create_graphics_pipeline(
                device, m_shadow_render_pass, 0 /*sub_pass*/, VK_SAMPLE_COUNT_1_BIT, m_shadow_pipeline_layout, m_shadow_width,
                m_shadow_height, shadow_shaders, vertex_binding_descriptions, vertex_attribute_descriptions, true /*for_shadow*/);

        LOG("Shadow depth attachment format " + format_to_string(m_shadow_depth_attachment->format()));
        LOG("Shadow zoom " + to_string_fixed(shadow_zoom, 5));
        if (old_shadow_width != m_shadow_width || old_shadow_height != m_shadow_height)
        {
                LOG("Requested shadow size (" + to_string(old_shadow_width) + ", " + to_string(old_shadow_height) +
                    "), selected shadow size (" + to_string(m_shadow_width) + ", " + to_string(m_shadow_height) + ")");
        }
}

const ShadowDepthAttachment* SwapchainAndBuffers::shadow_texture() const noexcept
{
        return m_shadow_depth_attachment.get();
}

void SwapchainAndBuffers::create_command_buffers(
        const Color& clear_color,
        const std::function<void(VkPipelineLayout pipeline_layout, VkPipeline pipeline, VkCommandBuffer command_buffer)>&
                commands_for_triangle_topology,
        const std::function<void(VkPipelineLayout pipeline_layout, VkPipeline pipeline, VkCommandBuffer command_buffer)>&
                commands_for_shadow_triangle_topology)
{
        if (m_sample_count_bit != VK_SAMPLE_COUNT_1_BIT)
        {
                std::array<VkClearValue, 3> clear_values;
                clear_values[0] = color_float_srgb_clear_value(clear_color);
                clear_values[1] = color_float_srgb_clear_value(clear_color);
                clear_values[2] = depth_stencil_clear_value();
                m_command_buffers = ::create_command_buffers(
                        m_device, m_swapchain->width(), m_swapchain->height(), m_render_pass, m_pipeline_layout, m_pipeline,
                        m_framebuffers, m_graphics_command_pool, clear_values, commands_for_triangle_topology);
        }
        else
        {
                std::array<VkClearValue, 2> clear_values;
                clear_values[0] = color_float_srgb_clear_value(clear_color);
                clear_values[1] = depth_stencil_clear_value();
                m_command_buffers = ::create_command_buffers(
                        m_device, m_swapchain->width(), m_swapchain->height(), m_render_pass, m_pipeline_layout, m_pipeline,
                        m_framebuffers, m_graphics_command_pool, clear_values, commands_for_triangle_topology);
        }

        //

        {
                std::array<VkClearValue, 1> clear_values;
                clear_values[0] = depth_stencil_clear_value();
                m_shadow_command_buffers =
                        ::create_command_buffers(m_device, m_shadow_width, m_shadow_height, m_shadow_render_pass,
                                                 m_shadow_pipeline_layout, m_shadow_pipeline, m_shadow_framebuffers,
                                                 m_graphics_command_pool, clear_values, commands_for_shadow_triangle_topology);
        }
}

void SwapchainAndBuffers::delete_command_buffers()
{
        m_command_buffers = CommandBuffers();
        m_shadow_command_buffers = CommandBuffers();
}

bool SwapchainAndBuffers::command_buffers_created() const
{
        return m_command_buffers.count() > 0 && m_shadow_command_buffers.count() > 0;
}

VkSwapchainKHR SwapchainAndBuffers::swapchain() const noexcept
{
        return m_swapchain->swapchain();
}

const VkCommandBuffer& SwapchainAndBuffers::command_buffer(uint32_t index) const noexcept
{
        return m_command_buffers[index];
}

const VkCommandBuffer& SwapchainAndBuffers::shadow_command_buffer() const noexcept
{
        ASSERT(m_shadow_command_buffers.count() == 1);

        return m_shadow_command_buffers[0];
}

//

VulkanInstance::VulkanInstance(int api_version_major, int api_version_minor,
                               const std::vector<std::string>& required_instance_extensions,
                               const std::vector<std::string>& required_device_extensions,
                               const std::vector<std::string>& required_validation_layers,
                               const std::vector<PhysicalDeviceFeatures>& required_features,
                               const std::vector<PhysicalDeviceFeatures>& optional_features,
                               const std::function<VkSurfaceKHR(VkInstance)>& create_surface, unsigned max_frames_in_flight)
        : m_instance(create_instance(api_version_major, api_version_minor, required_instance_extensions,
                                     required_validation_layers)),
          m_callback(!required_validation_layers.empty() ? std::make_optional(create_debug_report_callback(m_instance)) :
                                                           std::nullopt),
          m_surface(m_instance, create_surface),
          //
          m_physical_device(find_physical_device(m_instance, m_surface, api_version_major, api_version_minor,
                                                 required_device_extensions + VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                 required_features)),
          m_device(create_device(
                  m_physical_device,
                  {m_physical_device.graphics(), m_physical_device.compute(), m_physical_device.transfer(),
                   m_physical_device.presentation()},
                  required_device_extensions + VK_KHR_SWAPCHAIN_EXTENSION_NAME, required_validation_layers,
                  make_enabled_device_features(required_features, optional_features, m_physical_device.features()))),
          //
          m_max_frames_in_flight(max_frames_in_flight),
          m_image_available_semaphores(create_semaphores(m_device, m_max_frames_in_flight)),
          m_shadow_available_semaphores(create_semaphores(m_device, m_max_frames_in_flight)),
          m_render_finished_semaphores(create_semaphores(m_device, m_max_frames_in_flight)),
          m_in_flight_fences(create_fences(m_device, m_max_frames_in_flight, true /*signaled_state*/)),
          //
          m_graphics_command_pool(create_command_pool(m_device, m_physical_device.graphics())),
          m_graphics_queue(device_queue(m_device, m_physical_device.graphics(), 0 /*queue_index*/)),
          //
          m_transfer_command_pool(create_transient_command_pool(m_device, m_physical_device.transfer())),
          m_transfer_queue(device_queue(m_device, m_physical_device.transfer(), 0 /*queue_index*/)),
          //
          m_compute_queue(device_queue(m_device, m_physical_device.compute(), 0 /*queue_index*/)),
          m_presentation_queue(device_queue(m_device, m_physical_device.presentation(), 0 /*queue_index*/)),
          //
          m_buffer_family_indices(unique_elements(std::vector({m_physical_device.graphics(), m_physical_device.transfer()}))),
          m_swapchain_family_indices(
                  unique_elements(std::vector({m_physical_device.graphics(), m_physical_device.presentation()}))),
          m_texture_family_indices(unique_elements(std::vector({m_physical_device.graphics(), m_physical_device.transfer()}))),
          m_attachment_family_indices(unique_elements(std::vector({m_physical_device.graphics()})))
{
        if (max_frames_in_flight > 1)
        {
                error("Max frames in flight > 1 is not supported");
        }
}

VulkanInstance::~VulkanInstance()
{
        try
        {
                device_wait_idle();
        }
        catch (std::exception& e)
        {
                LOG(std::string("Device wait idle exception in the Vulkan instance destructor: ") + e.what());
        }
        catch (...)
        {
                LOG("Device wait idle unknown exception in the Vulkan instance destructor");
        }
}

VkInstance VulkanInstance::instance() const noexcept
{
        return m_instance;
}

const Device& VulkanInstance::device() const noexcept
{
        return m_device;
}

// return false - recreate swapchain
bool VulkanInstance::draw_frame(SwapchainAndBuffers& swapchain_and_buffers)
{
        constexpr VkFence NO_FENCE = VK_NULL_HANDLE;

        ASSERT(swapchain_and_buffers.command_buffers_created());

        VkResult result;

        const VkFence current_frame_fence = m_in_flight_fences[m_current_frame];

        result = vkWaitForFences(m_device, 1, &current_frame_fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkWaitForFences", result);
        }
        result = vkResetFences(m_device, 1, &current_frame_fence);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkResetFences", result);
        }

        //

        uint32_t image_index;
        result = vkAcquireNextImageKHR(m_device, swapchain_and_buffers.swapchain(), std::numeric_limits<uint64_t>::max(),
                                       m_image_available_semaphores[m_current_frame], NO_FENCE, &image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
                return false;
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
        }
        else if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkAcquireNextImageKHR", result);
        }

        //

        std::array<VkSemaphore, 1> render_finished_semaphores = {m_render_finished_semaphores[m_current_frame]};

        if (!m_draw_shadow)
        {
                std::array<VkSemaphore, 1> image_signal_semaphores = {m_image_available_semaphores[m_current_frame]};
                std::array<VkPipelineStageFlags, 1> image_wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

                VkSubmitInfo submit_info = {};
                submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.waitSemaphoreCount = image_signal_semaphores.size();
                submit_info.pWaitSemaphores = image_signal_semaphores.data();
                submit_info.pWaitDstStageMask = image_wait_stages.data();
                submit_info.commandBufferCount = 1;
                submit_info.pCommandBuffers = &swapchain_and_buffers.command_buffer(image_index);
                submit_info.signalSemaphoreCount = render_finished_semaphores.size();
                submit_info.pSignalSemaphores = render_finished_semaphores.data();

                result = vkQueueSubmit(m_graphics_queue, 1, &submit_info, current_frame_fence);
                if (result != VK_SUCCESS)
                {
                        vulkan_function_error("vkQueueSubmit", result);
                }
        }
        else
        {
                std::array<VkSemaphore, 1> shadow_signal_semaphores = {m_shadow_available_semaphores[m_current_frame]};

                std::array<VkSemaphore, 2> color_wait_semaphores = {m_shadow_available_semaphores[m_current_frame],
                                                                    m_image_available_semaphores[m_current_frame]};
                std::array<VkPipelineStageFlags, 2> color_wait_stages = {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

                {
                        VkSubmitInfo submit_info = {};
                        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                        submit_info.commandBufferCount = 1;
                        submit_info.pCommandBuffers = &swapchain_and_buffers.shadow_command_buffer();
                        submit_info.signalSemaphoreCount = shadow_signal_semaphores.size();
                        submit_info.pSignalSemaphores = shadow_signal_semaphores.data();

                        result = vkQueueSubmit(m_graphics_queue, 1, &submit_info, NO_FENCE);
                        if (result != VK_SUCCESS)
                        {
                                vulkan_function_error("vkQueueSubmit", result);
                        }
                }
                {
                        VkSubmitInfo submit_info = {};
                        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                        submit_info.waitSemaphoreCount = color_wait_semaphores.size();
                        submit_info.pWaitSemaphores = color_wait_semaphores.data();
                        submit_info.pWaitDstStageMask = color_wait_stages.data();
                        submit_info.commandBufferCount = 1;
                        submit_info.pCommandBuffers = &swapchain_and_buffers.command_buffer(image_index);
                        submit_info.signalSemaphoreCount = render_finished_semaphores.size();
                        submit_info.pSignalSemaphores = render_finished_semaphores.data();

                        result = vkQueueSubmit(m_graphics_queue, 1, &submit_info, current_frame_fence);
                        if (result != VK_SUCCESS)
                        {
                                vulkan_function_error("vkQueueSubmit", result);
                        }
                }
        }

        //

        std::array<VkSwapchainKHR, 1> swapchains = {swapchain_and_buffers.swapchain()};
        std::array<uint32_t, 1> image_indices = {image_index};

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = render_finished_semaphores.size();
        present_info.pWaitSemaphores = render_finished_semaphores.data();
        present_info.swapchainCount = swapchains.size();
        present_info.pSwapchains = swapchains.data();
        present_info.pImageIndices = image_indices.data();
        // present_info.pResults = nullptr;

        result = vkQueuePresentKHR(m_presentation_queue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
                return false;
        }
        else if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkQueuePresentKHR", result);
        }

        m_current_frame = (m_current_frame + 1) % m_max_frames_in_flight;

        return true;
}

void VulkanInstance::set_draw_shadow(bool draw)
{
        m_draw_shadow = draw;
}

void VulkanInstance::device_wait_idle() const
{
        ASSERT(m_device != VK_NULL_HANDLE);

        VkResult result = vkDeviceWaitIdle(m_device);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkDeviceWaitIdle", result);
        }
}
}
