/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "create.h"

#include "objects.h"
#include "strings.h"

#include <src/color/conversion.h>
#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
namespace
{
handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts,
        const std::vector<VkPushConstantRange>* const push_constant_ranges)
{
        VkPipelineLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        info.setLayoutCount = descriptor_set_layouts.size();
        info.pSetLayouts = descriptor_set_layouts.data();

        if (push_constant_ranges)
        {
                info.pushConstantRangeCount = push_constant_ranges->size();
                info.pPushConstantRanges = push_constant_ranges->data();
        }

        return {device, info};
}

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<unsigned>& set_numbers,
        const std::vector<VkDescriptorSetLayout>& set_layouts,
        const std::vector<VkPushConstantRange>* const push_constant_ranges)
{
        ASSERT(set_numbers.size() == set_layouts.size());
        ASSERT(!set_numbers.empty());
        ASSERT(set_numbers.size() == std::unordered_set(set_numbers.begin(), set_numbers.end()).size());
        ASSERT(0 == *std::min_element(set_numbers.begin(), set_numbers.end()));
        ASSERT(set_numbers.size() == 1 + *std::max_element(set_numbers.begin(), set_numbers.end()));

        std::vector<VkDescriptorSetLayout> layouts(set_numbers.size());
        auto n = set_numbers.begin();
        auto l = set_layouts.begin();
        while (n != set_numbers.end() && l != set_layouts.end())
        {
                layouts.at(*n++) = *l++;
        }

        ASSERT(n == set_numbers.end() && l == set_layouts.end());

        return create_pipeline_layout(device, layouts, push_constant_ranges);
}
}

//

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
{
        return create_pipeline_layout(device, descriptor_set_layouts, nullptr);
}

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts,
        const std::vector<VkPushConstantRange>& push_constant_ranges)
{
        return create_pipeline_layout(device, descriptor_set_layouts, &push_constant_ranges);
}

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<unsigned>& set_numbers,
        const std::vector<VkDescriptorSetLayout>& set_layouts)
{
        return create_pipeline_layout(device, set_numbers, set_layouts, nullptr);
}

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<unsigned>& set_numbers,
        const std::vector<VkDescriptorSetLayout>& set_layouts,
        const std::vector<VkPushConstantRange>& push_constant_ranges)
{
        return create_pipeline_layout(device, set_numbers, set_layouts, &push_constant_ranges);
}

//

std::vector<handle::Semaphore> create_semaphores(const VkDevice device, const int count)
{
        std::vector<handle::Semaphore> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device);
        }
        return res;
}

std::vector<handle::Fence> create_fences(const VkDevice device, const int count, const bool signaled_state)
{
        std::vector<handle::Fence> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device, signaled_state);
        }
        return res;
}

CommandPool create_command_pool(const VkDevice device, const std::uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = queue_family_index;

        return {device, info};
}

CommandPool create_transient_command_pool(const VkDevice device, const std::uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = queue_family_index;
        info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        return {device, info};
}

handle::Framebuffer create_framebuffer(
        const VkDevice device,
        const VkRenderPass render_pass,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::vector<VkImageView>& attachments)
{
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = render_pass;
        info.attachmentCount = attachments.size();
        info.pAttachments = attachments.data();
        info.width = width;
        info.height = height;
        info.layers = 1;

        return {device, info};
}

VkClearValue create_color_clear_value(const VkFormat format, const numerical::Vector<4, float>& rgba)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (format)
        {
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        {
                VkClearValue clear_value;
                clear_value.color.float32[0] = color::linear_float_to_srgb_float(rgba[0]);
                clear_value.color.float32[1] = color::linear_float_to_srgb_float(rgba[1]);
                clear_value.color.float32[2] = color::linear_float_to_srgb_float(rgba[2]);
                clear_value.color.float32[3] = rgba[3];
                return clear_value;
        }
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        {
                VkClearValue clear_value;
                clear_value.color.float32[0] = rgba[0];
                clear_value.color.float32[1] = rgba[1];
                clear_value.color.float32[2] = rgba[2];
                clear_value.color.float32[3] = rgba[3];
                return clear_value;
        }
        default:
                error("Unsupported format " + format_to_string(format) + " for color clear value");
        }
#pragma GCC diagnostic pop
}

VkClearValue create_color_clear_value(const VkFormat format, const numerical::Vector<3, float>& rgb)
{
        return create_color_clear_value(format, numerical::Vector<4, float>(rgb[0], rgb[1], rgb[2], 1));
}

VkClearValue create_depth_stencil_clear_value()
{
        VkClearValue clear_value;
        clear_value.depthStencil.depth = 1;
        clear_value.depthStencil.stencil = 0;
        return clear_value;
}
}
