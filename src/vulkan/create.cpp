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

#include "create.h"

#include "overview.h"
#include "print.h"
#include "query.h"
#include "settings.h"

#include <src/color/conversion.h>
#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/string/vector.h>
#include <src/settings/name.h>

#include <unordered_set>

namespace ns::vulkan
{
std::vector<Semaphore> create_semaphores(VkDevice device, int count)
{
        std::vector<Semaphore> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device);
        }
        return res;
}

std::vector<Fence> create_fences(VkDevice device, int count, bool signaled_state)
{
        std::vector<Fence> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device, signaled_state);
        }
        return res;
}

//

PipelineLayout create_pipeline_layout(VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
{
        VkPipelineLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = descriptor_set_layouts.size();
        create_info.pSetLayouts = descriptor_set_layouts.data();
        // create_info.pushConstantRangeCount = 0;
        // create_info.pPushConstantRanges = nullptr;

        return PipelineLayout(device, create_info);
}

PipelineLayout create_pipeline_layout(
        VkDevice device,
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

CommandPool create_command_pool(VkDevice device, uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;

        return CommandPool(device, create_info);
}

CommandPool create_transient_command_pool(VkDevice device, uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;
        create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        return CommandPool(device, create_info);
}

//

Instance create_instance(std::vector<std::string> required_extensions)
{
        LOG(overview());

        check_api_version(API_VERSION);

        if (!VALIDATION_LAYERS.empty())
        {
                required_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        sort_and_unique(&required_extensions);

        check_instance_extension_support(required_extensions);

        if (!VALIDATION_LAYERS.empty())
        {
                check_validation_layer_support({VALIDATION_LAYERS.cbegin(), VALIDATION_LAYERS.cend()});
        }

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = settings::APPLICATION_NAME;
        app_info.applicationVersion = 1;
        app_info.pEngineName = nullptr;
        app_info.engineVersion = 0;
        app_info.apiVersion = API_VERSION;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        const std::vector<const char*> extensions = const_char_pointer_vector(required_extensions);
        if (!extensions.empty())
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        if (!VALIDATION_LAYERS.empty())
        {
                create_info.enabledLayerCount = VALIDATION_LAYERS.size();
                create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }

        Instance instance(create_info);

        return instance;
}

//

Framebuffer create_framebuffer(
        VkDevice device,
        VkRenderPass render_pass,
        uint32_t width,
        uint32_t height,
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

        return Framebuffer(device, create_info);
}

//

VkClearValue color_clear_value(VkFormat format, VkColorSpaceKHR color_space, const Vector<3, float>& rgb)
{
        if (color_space == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
                if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_B8G8R8A8_UNORM)
                {
                        VkClearValue clear_value;
                        clear_value.color.float32[0] = color::linear_float_to_srgb_float(rgb[0]);
                        clear_value.color.float32[1] = color::linear_float_to_srgb_float(rgb[1]);
                        clear_value.color.float32[2] = color::linear_float_to_srgb_float(rgb[2]);
                        clear_value.color.float32[3] = 1;
                        return clear_value;
                }

                if (format == VK_FORMAT_R8G8B8A8_SRGB || format == VK_FORMAT_B8G8R8A8_SRGB)
                {
                        VkClearValue clear_value;
                        clear_value.color.float32[0] = rgb[0];
                        clear_value.color.float32[1] = rgb[1];
                        clear_value.color.float32[2] = rgb[2];
                        clear_value.color.float32[3] = 1;
                        return clear_value;
                }
        }

        error("Unsupported clear color format " + format_to_string(format) + " and color space "
              + color_space_to_string(color_space));
}

VkClearValue depth_stencil_clear_value()
{
        VkClearValue clear_value;
        clear_value.depthStencil.depth = 1;
        clear_value.depthStencil.stencil = 0;
        return clear_value;
}
}
