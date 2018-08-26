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

#include "com/error.h"

#include <vulkan/vulkan.h>

#if defined(VK_NO_PROTOTYPES)
#error VK_NO_PROTOTYPES defined
#endif

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                                              const VkAllocationCallbacks* pAllocator,
                                                              VkDebugReportCallbackEXT* pCallback)
{
        ASSERT(instance != VK_NULL_HANDLE);

        PFN_vkCreateDebugReportCallbackEXT f = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
                vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));

        if (!f)
        {
                error_fatal("vkCreateDebugReportCallbackEXT address not found");
        }

        return f(instance, pCreateInfo, pAllocator, pCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback,
                                                           const VkAllocationCallbacks* pAllocator)
{
        ASSERT(instance != VK_NULL_HANDLE);

        PFN_vkDestroyDebugReportCallbackEXT f = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));

        if (!f)
        {
                error_fatal("vkDestroyDebugReportCallbackEXT address not found");
        }

        f(instance, callback, pAllocator);
}
