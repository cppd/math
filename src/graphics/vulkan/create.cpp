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

#include "create.h"

#include "overview.h"
#include "print.h"
#include "query.h"

#include "application/name.h"
#include "com/error.h"
#include "com/log.h"
#include "com/string/vector.h"

#include <unordered_set>

namespace vulkan
{
std::vector<Semaphore> create_semaphores(VkDevice device, int count)
{
        std::vector<Semaphore> res;
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device);
        }
        return res;
}

std::vector<Fence> create_fences(VkDevice device, int count, bool signaled_state)
{
        std::vector<Fence> res;
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

PipelineLayout create_pipeline_layout(VkDevice device, const std::vector<unsigned>& set_numbers,
                                      const std::vector<VkDescriptorSetLayout>& set_layouts)
{
        ASSERT(set_numbers.size() == set_layouts.size() && set_numbers.size() > 0);
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

Instance create_instance(int api_version_major, int api_version_minor, std::vector<std::string> required_extensions,
                         const std::vector<std::string>& required_validation_layers)
{
        LOG(overview());

        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        if (required_validation_layers.size() > 0)
        {
                required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        check_api_version(required_api_version);
        check_instance_extension_support(required_extensions);
        check_validation_layer_support(required_validation_layers);

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

        Instance instance(create_info);

        return instance;
}

//

Framebuffer create_framebuffer(VkDevice device, VkRenderPass render_pass, uint32_t width, uint32_t height,
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

VkClearValue color_clear_value(VkFormat format, VkColorSpaceKHR color_space, const Color& color)
{
        if (color_space == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
                if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_B8G8R8A8_UNORM)
                {
                        VkClearValue clear_value;
                        clear_value.color.float32[0] = color_conversion::rgb_float_to_srgb_float(color.red());
                        clear_value.color.float32[1] = color_conversion::rgb_float_to_srgb_float(color.green());
                        clear_value.color.float32[2] = color_conversion::rgb_float_to_srgb_float(color.blue());
                        clear_value.color.float32[3] = 1;
                        return clear_value;
                }

                if (format == VK_FORMAT_R8G8B8A8_SRGB || format == VK_FORMAT_B8G8R8A8_SRGB)
                {
                        VkClearValue clear_value;
                        clear_value.color.float32[0] = color.red();
                        clear_value.color.float32[1] = color.green();
                        clear_value.color.float32[2] = color.blue();
                        clear_value.color.float32[3] = 1;
                        return clear_value;
                }
        }

        error("Unsupported clear color format " + format_to_string(format) + " and color space " +
              color_space_to_string(color_space));
}

VkClearValue depth_stencil_clear_value()
{
        VkClearValue clear_value;
        clear_value.depthStencil.depth = 1;
        clear_value.depthStencil.stencil = 0;
        return clear_value;
}

std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_info(const std::vector<const Shader*>& shaders)
{
        std::vector<VkPipelineShaderStageCreateInfo> res;

        for (const Shader* s : shaders)
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
}
