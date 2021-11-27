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

#include "debug.h"

#include <src/com/error.h>
#include <src/com/log.h>

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

bool bits(const VkDebugReportFlagsEXT flags, const VkDebugReportFlagBitsEXT bits)
{
        return (flags & bits) == bits;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        const VkDebugReportFlagsEXT flags,
        const VkDebugReportObjectTypeEXT /*object_type*/,
        const std::uint64_t /*object*/,
        const std::size_t /*location*/,
        const std::int32_t /*message_code*/,
        const char* const /*layer_prefix*/,
        const char* const message,
        void* const /*user_data*/)
{
        std::string s;

        if (bits(flags, VK_DEBUG_REPORT_INFORMATION_BIT_EXT))
        {
                add(&s, "information");
        }
        if (bits(flags, VK_DEBUG_REPORT_WARNING_BIT_EXT))
        {
                add(&s, "warning");
        }
        if (bits(flags, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
        {
                add(&s, "performance warning");
        }
        if (bits(flags, VK_DEBUG_REPORT_ERROR_BIT_EXT))
        {
                add(&s, "error");
        }
        if (bits(flags, VK_DEBUG_REPORT_DEBUG_BIT_EXT))
        {
                add(&s, "debug");
        }

        if (!s.empty())
        {
                LOG("Validation layer message (" + s + "): " + message);
        }
        else
        {
                LOG(std::string("Validation layer message: ") + message);
        }

        return VK_FALSE;
}
}

handle::DebugReportCallbackEXT create_debug_report_callback(const VkInstance instance)
{
        if (instance == VK_NULL_HANDLE)
        {
                error("No VkInstance for DebugReportCallbackEXT");
        }

        VkDebugReportCallbackCreateInfoEXT create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;

        create_info.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
        create_info.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
        create_info.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

        // create_info.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        // create_info.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;

        create_info.pfnCallback = debug_callback;

        return handle::DebugReportCallbackEXT(instance, create_info);
}
}
