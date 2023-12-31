/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "render_pass.h"

#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <array>

namespace ns::view
{
vulkan::RenderPass render_pass_swapchain_color(
        const VkDevice device,
        const VkFormat color_format,
        const VkSampleCountFlagBits sample_count)
{
        const std::array<VkAttachmentDescription, 2> attachments = [&]
        {
                std::array<VkAttachmentDescription, 2> res = {};

                // Color resolve
                res[0].format = color_format;
                res[0].samples = VK_SAMPLE_COUNT_1_BIT;
                res[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                res[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                res[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                res[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                res[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                res[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                // Color
                res[1].format = color_format;
                res[1].samples = sample_count;
                res[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                res[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                res[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                res[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                res[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                res[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                return res;
        }();

        const VkAttachmentReference color_reference = [&]
        {
                VkAttachmentReference res = {};
                res.attachment = 1;
                res.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                return res;
        }();

        const VkAttachmentReference color_resolve_reference = [&]
        {
                VkAttachmentReference res = {};
                res.attachment = 0;
                res.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                return res;
        }();

        const VkSubpassDescription subpass_description = [&]
        {
                VkSubpassDescription res = {};
                res.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                res.colorAttachmentCount = 1;
                res.pColorAttachments = &color_reference;
                res.pResolveAttachments = &color_resolve_reference;
                return res;
        }();

        const std::array<VkSubpassDependency, 1> subpass_dependencies = [&]
        {
                std::array<VkSubpassDependency, 1> res = {};
                res[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                res[0].dstSubpass = 0;
                res[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].srcAccessMask = 0;
                res[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                return res;
        }();

        const VkRenderPassCreateInfo create_info = [&]
        {
                VkRenderPassCreateInfo res = {};
                res.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                res.attachmentCount = attachments.size();
                res.pAttachments = attachments.data();
                res.subpassCount = 1;
                res.pSubpasses = &subpass_description;
                res.dependencyCount = subpass_dependencies.size();
                res.pDependencies = subpass_dependencies.data();
                return res;
        }();

        return {device, create_info};
}

vulkan::RenderPass render_pass_color_depth(
        const VkDevice device,
        const VkFormat color_format,
        const VkFormat depth_format,
        const VkSampleCountFlagBits sample_count,
        const bool clear)
{
        const std::array<VkAttachmentDescription, 2> attachments = [&]
        {
                std::array<VkAttachmentDescription, 2> res = {};

                // Color
                res[0].format = color_format;
                res[0].samples = sample_count;
                res[0].loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                res[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                res[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                res[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                res[0].initialLayout = clear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                res[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                // Depth
                res[1].format = depth_format;
                res[1].samples = sample_count;
                res[1].loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                res[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                res[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                res[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                res[1].initialLayout =
                        clear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                res[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                return res;
        }();

        const VkAttachmentReference color_reference = [&]
        {
                VkAttachmentReference res = {};
                res.attachment = 0;
                res.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                return res;
        }();

        const VkAttachmentReference depth_reference = [&]
        {
                VkAttachmentReference res = {};
                res.attachment = 1;
                res.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                return res;
        }();

        const VkSubpassDescription subpass_description = [&]
        {
                VkSubpassDescription res = {};
                res.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                res.colorAttachmentCount = 1;
                res.pColorAttachments = &color_reference;
                res.pDepthStencilAttachment = &depth_reference;
                return res;
        }();

#if 1
        const std::array<VkSubpassDependency, 1> subpass_dependencies = [&]
        {
                std::array<VkSubpassDependency, 1> res = {};
                res[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                res[0].dstSubpass = 0;
                res[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].srcAccessMask = 0;
                res[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                return res;
        }();
#else
        const std::array<VkSubpassDependency, 2> subpass_dependencies = [&]
        {
                std::array<VkSubpassDependency, 2> res = {};

                res[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                res[0].dstSubpass = 0;
                res[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                res[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].srcAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
                res[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                res[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                res[1].srcSubpass = 0;
                res[1].dstSubpass = VK_SUBPASS_EXTERNAL;
                res[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                res[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                res[1].dstAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT;
                res[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                return res;
        }();
#endif

        const VkRenderPassCreateInfo create_info = [&]
        {
                VkRenderPassCreateInfo res = {};
                res.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                res.attachmentCount = attachments.size();
                res.pAttachments = attachments.data();
                res.subpassCount = 1;
                res.pSubpasses = &subpass_description;
                res.dependencyCount = subpass_dependencies.size();
                res.pDependencies = subpass_dependencies.data();
                return res;
        }();

        return {device, create_info};
}

vulkan::RenderPass render_pass_color(
        const VkDevice device,
        const VkFormat color_format,
        const VkSampleCountFlagBits sample_count)
{
        const std::array<VkAttachmentDescription, 1> attachments = [&]
        {
                std::array<VkAttachmentDescription, 1> res = {};

                // Color
                res[0].format = color_format;
                res[0].samples = sample_count;
                res[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                res[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                res[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                res[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                res[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                res[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                return res;
        }();

        const VkAttachmentReference color_reference = [&]
        {
                VkAttachmentReference res = {};
                res.attachment = 0;
                res.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                return res;
        }();

        const VkSubpassDescription subpass_description = [&]
        {
                VkSubpassDescription res = {};
                res.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                res.colorAttachmentCount = 1;
                res.pColorAttachments = &color_reference;
                return res;
        }();

        const std::array<VkSubpassDependency, 1> subpass_dependencies = [&]
        {
                std::array<VkSubpassDependency, 1> res = {};
                res[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                res[0].dstSubpass = 0;
                res[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                res[0].srcAccessMask = 0;
                res[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                return res;
        }();

        const VkRenderPassCreateInfo create_info = [&]
        {
                VkRenderPassCreateInfo res = {};
                res.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                res.attachmentCount = attachments.size();
                res.pAttachments = attachments.data();
                res.subpassCount = 1;
                res.pSubpasses = &subpass_description;
                res.dependencyCount = subpass_dependencies.size();
                res.pDependencies = subpass_dependencies.data();
                return res;
        }();

        return {device, create_info};
}
}
