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

#include "create.h"

#include "print.h"

#include <src/color/conversion.h>
#include <src/com/error.h>

#include <unordered_set>

namespace ns::vulkan
{
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

//

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
{
        VkPipelineLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = descriptor_set_layouts.size();
        create_info.pSetLayouts = descriptor_set_layouts.data();
        // create_info.pushConstantRangeCount = 0;
        // create_info.pPushConstantRanges = nullptr;

        return handle::PipelineLayout(device, create_info);
}

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<unsigned>& set_numbers,
        const std::vector<VkDescriptorSetLayout>& set_layouts)
{
        ASSERT(set_numbers.size() == set_layouts.size() && !set_numbers.empty());
        ASSERT(set_numbers.size() == std::unordered_set<unsigned>(set_numbers.begin(), set_numbers.end()).size());
        ASSERT(0 == *std::min_element(set_numbers.begin(), set_numbers.end()));
        ASSERT(set_numbers.size() - 1 == *std::max_element(set_numbers.begin(), set_numbers.end()));

        std::vector<VkDescriptorSetLayout> layouts(set_numbers.size());
        auto n = set_numbers.begin();
        auto l = set_layouts.begin();
        while (n != set_numbers.end() && l != set_layouts.end())
        {
                layouts.at(*n++) = *l++;
        }

        ASSERT(n == set_numbers.end() && l == set_layouts.end());

        return create_pipeline_layout(device, layouts);
}

//

CommandPool create_command_pool(const VkDevice device, const std::uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;

        return CommandPool(device, create_info);
}

CommandPool create_transient_command_pool(const VkDevice device, const std::uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;
        create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        return CommandPool(device, create_info);
}

//

handle::Framebuffer create_framebuffer(
        const VkDevice device,
        const VkRenderPass render_pass,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::vector<VkImageView>& attachments)
{
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.width = width;
        create_info.height = height;
        create_info.layers = 1;

        return handle::Framebuffer(device, create_info);
}

//

VkClearValue create_color_clear_value(const VkFormat format, const Vector<3, float>& rgb)
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
                clear_value.color.float32[0] = color::linear_float_to_srgb_float(rgb[0]);
                clear_value.color.float32[1] = color::linear_float_to_srgb_float(rgb[1]);
                clear_value.color.float32[2] = color::linear_float_to_srgb_float(rgb[2]);
                clear_value.color.float32[3] = 1;
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
                clear_value.color.float32[0] = rgb[0];
                clear_value.color.float32[1] = rgb[1];
                clear_value.color.float32[2] = rgb[2];
                clear_value.color.float32[3] = 1;
                return clear_value;
        }
        default:
                error("Unsupported format " + format_to_string(format) + " for color clear value");
        }
#pragma GCC diagnostic pop
}

VkClearValue create_depth_stencil_clear_value()
{
        VkClearValue clear_value;
        clear_value.depthStencil.depth = 1;
        clear_value.depthStencil.stencil = 0;
        return clear_value;
}
}
