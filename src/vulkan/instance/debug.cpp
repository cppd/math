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

#include "debug.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/vulkan/objects.h>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include <string>

namespace ns::vulkan
{
namespace
{
void add(std::string* const str, const char* const text)
{
        if (!str->empty())
        {
                *str += ", ";
        }
        *str += text;
}

bool bits(const VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessageTypeFlagBitsEXT bits)
{
        return (flags & bits) == bits;
}

VkBool32 VKAPI_PTR user_callback(
        const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        const VkDebugUtilsMessageTypeFlagsEXT message_types,
        const VkDebugUtilsMessengerCallbackDataEXT* const callback_data,
        void* const /*user_data*/)
{
        std::string s;

        if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
                add(&s, "error");
        }
        else if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
                add(&s, "warning");
        }
        else if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
                add(&s, "info");
        }
        else if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
                add(&s, "verbose");
        }

        if (bits(message_types, VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        {
                add(&s, "performance");
        }
        if (bits(message_types, VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT))
        {
                add(&s, "validation");
        }
        if (bits(message_types, VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT))
        {
                add(&s, "general");
        }

        std::string msg = "Debug message";
        if (!s.empty())
        {
                msg += " (" + s + ")";
        }

        LOG(msg + ": " + callback_data->pMessage);

        return VK_FALSE;
}
}

handle::DebugUtilsMessengerEXT create_debug_utils_messenger(const VkInstance instance)
{
        if (instance == VK_NULL_HANDLE)
        {
                error("No VkInstance for DebugUtilsMessengerEXT");
        }

        VkDebugUtilsMessengerCreateInfoEXT info = {};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        info.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        info.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        info.pfnUserCallback = user_callback;

        return {instance, info};
}
}
