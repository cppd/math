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

#include "com/error.h"

#include <vulkan/vulkan.h>

#if defined(VK_NO_PROTOTYPES)
#error VK_NO_PROTOTYPES defined
#endif

#define GET_PROC_ADDR(name, instance)                                                                         \
        [](VkInstance instance_local_parameter) -> PFN_##name                                                 \
        {                                                                                                     \
                ASSERT(instance_local_parameter != VK_NULL_HANDLE);                                           \
                PFN_##name function_address_local_parameter =                                                 \
                        reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance_local_parameter, #name)); \
                if (!function_address_local_parameter)                                                        \
                {                                                                                             \
                        error("Failed to find address of " #name);                                            \
                }                                                                                             \
                return function_address_local_parameter;                                                      \
        }                                                                                                     \
        (instance)

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                                              const VkAllocationCallbacks* pAllocator,
                                                              VkDebugReportCallbackEXT* pCallback)
{
        auto f = GET_PROC_ADDR(vkCreateDebugReportCallbackEXT, instance);

        return f(instance, pCreateInfo, pAllocator, pCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback,
                                                           const VkAllocationCallbacks* pAllocator)
{
        auto f = GET_PROC_ADDR(vkDestroyDebugReportCallbackEXT, instance);

        f(instance, callback, pAllocator);
}
