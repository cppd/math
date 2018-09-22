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
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"

#include <cstring>

namespace
{
VkClearValue color_float_srgb_clear_value(const Color& clear_color)
{
        VkClearValue clear_value;
        clear_value.color.float32[0] = Color::rgb_float_to_srgb_float(clear_color.red());
        clear_value.color.float32[1] = Color::rgb_float_to_srgb_float(clear_color.green());
        clear_value.color.float32[2] = Color::rgb_float_to_srgb_float(clear_color.blue());
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

VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& surface_formats)
{
        ASSERT(surface_formats.size() > 0);

        if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
        {
                return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        }

        for (const VkSurfaceFormatKHR& format : surface_formats)
        {
                if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                        return format;
                }
        }

        error("Surface format (VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) is not found");
}

VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& present_modes)
{
        for (const VkPresentModeKHR& present_mode : present_modes)
        {
                if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                        return present_mode;
                }
        }

        for (const VkPresentModeKHR& present_mode : present_modes)
        {
                if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                        return present_mode;
                }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
        if (!(capabilities.currentExtent.width == 0xffff'ffff && capabilities.currentExtent.height == 0xffff'ffff))
        {
                return capabilities.currentExtent;
        }

        error("Current width and height of the surface are not defined");

#if 0
        VkExtent2D extent;
        extent.width = std::clamp(1u, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(1u, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return extent;
#endif
}

uint32_t choose_image_count(const VkSurfaceCapabilitiesKHR& capabilities, int image_count)
{
        if (image_count <= 0)
        {
                error("Requested image count <= 0");
        }

        if (static_cast<uint32_t>(image_count) <= capabilities.minImageCount)
        {
                return capabilities.minImageCount;
        }
        if (capabilities.maxImageCount > 0 && static_cast<uint32_t>(image_count) >= capabilities.maxImageCount)
        {
                return capabilities.maxImageCount;
        }

        return image_count;
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

        const std::vector<const char*> extensions = to_char_pointer_vector(required_extensions);
        if (extensions.size() > 0)
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        const std::vector<const char*> validation_layers = to_char_pointer_vector(required_validation_layers);
        if (validation_layers.size() > 0)
        {
                create_info.enabledLayerCount = validation_layers.size();
                create_info.ppEnabledLayerNames = validation_layers.data();
        }

        return vulkan::Instance(create_info);
}

vulkan::Device create_device(VkPhysicalDevice physical_device, const std::vector<uint32_t>& family_indices,
                             const std::vector<std::string>& required_extensions,
                             const std::vector<std::string>& required_validation_layers)
{
        if (family_indices.empty())
        {
                error("No family indices for device creation");
        }

        constexpr float queue_priority = 1;
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        for (uint32_t unique_queue_family_index : std::unordered_set<uint32_t>(family_indices.cbegin(), family_indices.cend()))
        {
                VkDeviceQueueCreateInfo queue_create_info = {};
                queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.queueFamilyIndex = unique_queue_family_index;
                queue_create_info.queueCount = 1;
                queue_create_info.pQueuePriorities = &queue_priority;

                queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features = {};
        device_features.samplerAnisotropy = VK_TRUE;
        device_features.geometryShader = VK_TRUE;
        device_features.sampleRateShading = VK_TRUE;

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &device_features;

        const std::vector<const char*> extensions = to_char_pointer_vector(required_extensions);
        if (extensions.size() > 0)
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        const std::vector<const char*> validation_layers = to_char_pointer_vector(required_validation_layers);
        if (validation_layers.size() > 0)
        {
                create_info.enabledLayerCount = validation_layers.size();
                create_info.ppEnabledLayerNames = validation_layers.data();
        }

        return vulkan::Device(physical_device, create_info);
}

vulkan::SwapChainKHR create_swap_chain_khr(VkDevice device, VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format,
                                           VkPresentModeKHR present_mode, VkExtent2D extent, uint32_t image_count,
                                           VkSurfaceTransformFlagBitsKHR transform, const std::vector<uint32_t>& family_indices)
{
        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface;

        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        ASSERT(family_indices.size() > 0);
        ASSERT(family_indices.size() == std::unordered_set<uint32_t>(family_indices.cbegin(), family_indices.cend()).size());

        if (family_indices.size() > 1)
        {
                create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = family_indices.size();
                create_info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        create_info.preTransform = transform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;

        create_info.oldSwapchain = VK_NULL_HANDLE;

        return vulkan::SwapChainKHR(device, create_info);
}

vulkan::ImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = image;

        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = aspect_flags;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        return vulkan::ImageView(device, create_info);
}

vulkan::RenderPass create_render_pass(VkDevice device, VkFormat swap_chain_image_format, VkFormat depth_image_format)
{
        std::array<VkAttachmentDescription, 2> attachments = {};

        // Color
        attachments[0].format = swap_chain_image_format;
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
                                                    VkFormat swap_chain_image_format, VkFormat depth_image_format)
{
        std::array<VkAttachmentDescription, 3> attachments = {};

        // Color resolve
        attachments[0].format = swap_chain_image_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Multisampling color
        attachments[1].format = swap_chain_image_format;
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

template <size_t N>
vulkan::Framebuffer create_framebuffer(VkDevice device, VkRenderPass render_pass, VkExtent2D extent,
                                       const std::array<VkImageView, N>& attachments)
{
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.width = extent.width;
        create_info.height = extent.height;
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

vulkan::Pipeline create_graphics_pipeline(VkDevice device, VkRenderPass render_pass, uint32_t sub_pass,
                                          VkSampleCountFlagBits sample_count, VkPipelineLayout pipeline_layout,
                                          VkExtent2D swap_chain_extent, const std::vector<const vulkan::Shader*>& shaders,
                                          const std::vector<VkVertexInputBindingDescription>& binding_descriptions,
                                          const std::vector<VkVertexInputAttributeDescription>& attribute_descriptions)
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
        viewport.width = swap_chain_extent.width;
        viewport.height = swap_chain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = swap_chain_extent;

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

        rasterization_state_info.depthBiasEnable = VK_FALSE;
        // rasterization_state_info.depthBiasConstantFactor = 0.0f;
        // rasterization_state_info.depthBiasClamp = 0.0f;
        // rasterization_state_info.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling_state_info = {};
        multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling_state_info.rasterizationSamples = sample_count;
        multisampling_state_info.sampleShadingEnable = VK_TRUE;
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

        // std::vector<VkDynamicState> dynamic_states({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH});
        // VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
        // dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        // dynamic_state_info.dynamicStateCount = dynamic_states.size();
        // dynamic_state_info.pDynamicStates = dynamic_states.data();

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
        // create_info.pDynamicState = nullptr;

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
        VkDevice device, const VkExtent2D& swap_chain_extent, VkRenderPass render_pass, VkPipelineLayout pipeline_layout,
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
                render_pass_info.renderArea.extent = swap_chain_extent;
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

VkQueue device_queue(VkDevice device, uint32_t queue_family_index, uint32_t queue_index)
{
        VkQueue queue;
        vkGetDeviceQueue(device, queue_family_index, queue_index, &queue);
        ASSERT(queue != VK_NULL_HANDLE);
        return queue;
}
}

namespace vulkan
{
SwapChain::SwapChain(VkSurfaceKHR surface, VkPhysicalDevice physical_device,
                     const std::vector<uint32_t>& swap_chain_image_family_indices,
                     const std::vector<uint32_t>& depth_image_family_indices, const Device& device,
                     VkCommandPool graphics_command_pool, VkQueue graphics_queue, int preferred_image_count,
                     int required_minimum_sample_count, const std::vector<const vulkan::Shader*>& shaders,
                     const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                     const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
                     const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
        : m_device(device), m_graphics_command_pool(graphics_command_pool)
{
        ASSERT(surface != VK_NULL_HANDLE);
        ASSERT(physical_device != VK_NULL_HANDLE);
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(graphics_command_pool != VK_NULL_HANDLE);
        ASSERT(graphics_queue != VK_NULL_HANDLE);

        ASSERT(descriptor_set_layouts.size() > 0);
        ASSERT(swap_chain_image_family_indices.size() > 0);
        ASSERT(depth_image_family_indices.size() > 0);

        SwapChainDetails swap_chain_details;
        if (!find_swap_chain_details(surface, physical_device, &swap_chain_details))
        {
                error("Failed to find swap chain details");
        }

        const VkSurfaceFormatKHR surface_format = choose_surface_format(swap_chain_details.surface_formats);
        m_extent = choose_extent(swap_chain_details.surface_capabilities);
        const VkPresentModeKHR present_mode = choose_present_mode(swap_chain_details.present_modes);
        const uint32_t image_count = choose_image_count(swap_chain_details.surface_capabilities, preferred_image_count);

        const uint32_t sample_count = supported_framebuffer_sample_count(physical_device, required_minimum_sample_count);
        m_sample_count_bit = sample_count_flag_bit(sample_count);

        LOG("Preferred image count = " + to_string(preferred_image_count) + ", chosen image count = " + to_string(image_count) +
            "\n" + "Minimum sample count = " + to_string(required_minimum_sample_count) +
            ", chosen sample count = " + to_string(sample_count));

        m_swap_chain =
                create_swap_chain_khr(device, surface, surface_format, present_mode, m_extent, image_count,
                                      swap_chain_details.surface_capabilities.currentTransform, swap_chain_image_family_indices);

        m_swap_chain_images = swap_chain_images(device, m_swap_chain);
        if (m_swap_chain_images.empty())
        {
                error("Failed to find swap chain images");
        }

        for (const VkImage& image : m_swap_chain_images)
        {
                m_swap_chain_image_views.push_back(
                        create_image_view(device, image, surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT));
        }

        if (m_sample_count_bit != VK_SAMPLE_COUNT_1_BIT)
        {
                m_multisampling_color_attachment = std::make_unique<ColorAttachment>(
                        device, graphics_command_pool, graphics_queue, depth_image_family_indices, surface_format.format,
                        m_sample_count_bit, m_extent.width, m_extent.height);

                m_multisampling_depth_attachment = std::make_unique<DepthAttachment>(
                        device, graphics_command_pool, graphics_queue, depth_image_family_indices, m_sample_count_bit,
                        m_extent.width, m_extent.height);

                m_render_pass = create_multisampling_render_pass(device, m_sample_count_bit, surface_format.format,
                                                                 m_multisampling_depth_attachment->format());

                for (VkImageView swap_chain_image_view : m_swap_chain_image_views)
                {
                        std::array<VkImageView, 3> attachments;
                        attachments[0] = swap_chain_image_view;
                        attachments[1] = m_multisampling_color_attachment->image_view();
                        attachments[2] = m_multisampling_depth_attachment->image_view();

                        m_framebuffers.push_back(create_framebuffer(device, m_render_pass, m_extent, attachments));
                }
        }
        else
        {
                m_depth_attachment = std::make_unique<DepthAttachment>(device, graphics_command_pool, graphics_queue,
                                                                       depth_image_family_indices, VK_SAMPLE_COUNT_1_BIT,
                                                                       m_extent.width, m_extent.height);

                m_render_pass = create_render_pass(device, surface_format.format, m_depth_attachment->format());

                for (VkImageView swap_chain_image_view : m_swap_chain_image_views)
                {
                        std::array<VkImageView, 2> attachments;
                        attachments[0] = swap_chain_image_view;
                        attachments[1] = m_depth_attachment->image_view();

                        m_framebuffers.push_back(create_framebuffer(device, m_render_pass, m_extent, attachments));
                }
        }

        m_pipeline_layout = create_pipeline_layout(device, descriptor_set_layouts);
        m_pipeline = create_graphics_pipeline(device, m_render_pass, 0 /*sub_pass*/, m_sample_count_bit, m_pipeline_layout,
                                              m_extent, shaders, vertex_binding_descriptions, vertex_attribute_descriptions);
}

void SwapChain::create_command_buffers(const Color& clear_color,
                                       const std::function<void(VkPipelineLayout pipeline_layout, VkPipeline pipeline,
                                                                VkCommandBuffer command_buffer)>& commands_for_triangle_topology)
{
        if (m_sample_count_bit != VK_SAMPLE_COUNT_1_BIT)
        {
                std::array<VkClearValue, 3> clear_values;
                clear_values[0] = color_float_srgb_clear_value(clear_color);
                clear_values[1] = color_float_srgb_clear_value(clear_color);
                clear_values[2] = depth_stencil_clear_value();
                m_command_buffers =
                        ::create_command_buffers(m_device, m_extent, m_render_pass, m_pipeline_layout, m_pipeline, m_framebuffers,
                                                 m_graphics_command_pool, clear_values, commands_for_triangle_topology);
        }
        else
        {
                std::array<VkClearValue, 2> clear_values;
                clear_values[0] = color_float_srgb_clear_value(clear_color);
                clear_values[1] = depth_stencil_clear_value();
                m_command_buffers =
                        ::create_command_buffers(m_device, m_extent, m_render_pass, m_pipeline_layout, m_pipeline, m_framebuffers,
                                                 m_graphics_command_pool, clear_values, commands_for_triangle_topology);
        }
}

void SwapChain::delete_command_buffers()
{
        m_command_buffers = CommandBuffers();
}

bool SwapChain::command_buffers_created() const
{
        return m_command_buffers.count() > 0;
}

VkSwapchainKHR SwapChain::swap_chain() const noexcept
{
        return m_swap_chain;
}

const VkCommandBuffer& SwapChain::command_buffer(uint32_t index) const noexcept
{
        return m_command_buffers[index];
}

//

VulkanInstance::VulkanInstance(int api_version_major, int api_version_minor,
                               const std::vector<std::string>& required_instance_extensions,
                               const std::vector<std::string>& required_device_extensions,
                               const std::vector<std::string>& required_validation_layers,
                               const std::function<VkSurfaceKHR(VkInstance)>& create_surface, unsigned max_frames_in_flight)
        : m_instance(create_instance(api_version_major, api_version_minor, required_instance_extensions,
                                     required_validation_layers)),
          m_callback(!required_validation_layers.empty() ? std::make_optional(create_debug_report_callback(m_instance)) :
                                                           std::nullopt),
          m_surface(m_instance, create_surface),
          //
          m_physical_device(find_physical_device(m_instance, m_surface, api_version_major, api_version_minor,
                                                 required_device_extensions + VK_KHR_SWAPCHAIN_EXTENSION_NAME)),
          m_device(create_device(m_physical_device.device,
                                 {m_physical_device.graphics, m_physical_device.compute, m_physical_device.transfer,
                                  m_physical_device.presentation},
                                 required_device_extensions + VK_KHR_SWAPCHAIN_EXTENSION_NAME, required_validation_layers)),
          //
          m_max_frames_in_flight(max_frames_in_flight),
          m_image_available_semaphores(create_semaphores(m_device, m_max_frames_in_flight)),
          m_render_finished_semaphores(create_semaphores(m_device, m_max_frames_in_flight)),
          m_in_flight_fences(create_fences(m_device, m_max_frames_in_flight, true /*signaled_state*/)),
          //
          m_graphics_command_pool(create_command_pool(m_device, m_physical_device.graphics)),
          m_graphics_queue(device_queue(m_device, m_physical_device.graphics, 0 /*queue_index*/)),
          //
          m_transfer_command_pool(create_transient_command_pool(m_device, m_physical_device.transfer)),
          m_transfer_queue(device_queue(m_device, m_physical_device.transfer, 0 /*queue_index*/)),
          //
          m_compute_queue(device_queue(m_device, m_physical_device.compute, 0 /*queue_index*/)),
          m_presentation_queue(device_queue(m_device, m_physical_device.presentation, 0 /*queue_index*/)),
          //
          m_vertex_buffer_family_indices(unique_elements(std::vector({m_physical_device.graphics, m_physical_device.transfer}))),
          m_swap_chain_image_family_indices(
                  unique_elements(std::vector({m_physical_device.graphics, m_physical_device.presentation}))),
          m_texture_image_family_indices(unique_elements(std::vector({m_physical_device.graphics, m_physical_device.transfer}))),
          m_depth_image_family_indices(unique_elements(std::vector({m_physical_device.graphics})))
{
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

SwapChain VulkanInstance::create_swap_chain(int preferred_image_count, int required_minimum_sample_count,
                                            const std::vector<const vulkan::Shader*>& shaders,
                                            const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                            const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
                                            const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
{
        ASSERT(descriptor_set_layouts.size() > 0);

        return SwapChain(m_surface, m_physical_device.device, m_swap_chain_image_family_indices, m_depth_image_family_indices,
                         m_device, m_graphics_command_pool, m_graphics_queue, preferred_image_count,
                         required_minimum_sample_count, shaders, vertex_binding_descriptions, vertex_attribute_descriptions,
                         descriptor_set_layouts);
}

Texture VulkanInstance::create_texture(uint32_t width, uint32_t height, const std::vector<unsigned char>& rgba_pixels) const
{
        return Texture(m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool, m_transfer_queue,
                       m_texture_image_family_indices, width, height, rgba_pixels);
}

VkInstance VulkanInstance::instance() const noexcept
{
        return m_instance;
}

const Device& VulkanInstance::device() const noexcept
{
        return m_device;
}

// false - recreate swap chain
bool VulkanInstance::draw_frame(SwapChain& swap_chain)
{
        constexpr VkFence NO_FENCE = VK_NULL_HANDLE;

        ASSERT(swap_chain.command_buffers_created());

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
        result = vkAcquireNextImageKHR(m_device, swap_chain.swap_chain(), std::numeric_limits<uint64_t>::max(),
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

        std::array<VkSemaphore, 1> wait_semaphores = {m_image_available_semaphores[m_current_frame]};
        std::array<VkPipelineStageFlags, 1> wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        std::array<VkSemaphore, 1> signal_semaphores = {m_render_finished_semaphores[m_current_frame]};

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = wait_semaphores.size();
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &swap_chain.command_buffer(image_index);
        submit_info.signalSemaphoreCount = signal_semaphores.size();
        submit_info.pSignalSemaphores = signal_semaphores.data();

        result = vkQueueSubmit(m_graphics_queue, 1, &submit_info, current_frame_fence);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkQueueSubmit", result);
        }

        //

        std::array<VkSwapchainKHR, 1> swap_chains = {swap_chain.swap_chain()};
        std::array<uint32_t, 1> image_indices = {image_index};

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = signal_semaphores.size();
        present_info.pWaitSemaphores = signal_semaphores.data();
        present_info.swapchainCount = swap_chains.size();
        present_info.pSwapchains = swap_chains.data();
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
