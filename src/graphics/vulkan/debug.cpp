/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/error.h"
#include "com/log.h"

namespace
{
void add_to_debug_message(std::string* str, const char* text)
{
        if (!str->empty())
        {
                *str += ", ";
        }
        *str += text;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*objectType*/,
                                              uint64_t /*object*/, size_t /*location*/, int32_t /*messageCode*/,
                                              const char* /*pLayerPrefix*/, const char* pMessage, void* /*pUserData*/)
{
        std::string s;

        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
                add_to_debug_message(&s, "information");
        }
        if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
                add_to_debug_message(&s, "warning");
        }
        if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
                add_to_debug_message(&s, "performance warning");
        }
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
                add_to_debug_message(&s, "error");
        }
        if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
                add_to_debug_message(&s, "debug");
        }

        if (s.size() > 0)
        {
                LOG("Validation layer message (" + s + "): " + std::string(pMessage));
        }
        else
        {
                LOG("Validation layer message: " + std::string(pMessage));
        }

        return VK_FALSE;
}
}

namespace vulkan
{
DebugReportCallback create_debug_report_callback(VkInstance instance)
{
        if (instance == VK_NULL_HANDLE)
        {
                error("No VkInstance for DebugReportCallback");
        }

        VkDebugReportCallbackCreateInfoEXT create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;

        create_info.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
        create_info.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
        create_info.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
#if 0
        create_info.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        create_info.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
#endif

        create_info.pfnCallback = debug_callback;

        return DebugReportCallback(instance, create_info);
}
}
