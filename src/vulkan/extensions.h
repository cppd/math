/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <vulkan/vulkan.h>

namespace ns::vulkan
{
PFN_vkVoidFunction proc_addr(VkInstance instance, const char* name);
}

inline VkResult vkCreateDebugReportCallbackEXT(
        VkInstance instance,
        const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugReportCallbackEXT* pCallback)
{
        auto a = ns::vulkan::proc_addr(instance, "vkCreateDebugReportCallbackEXT");
        auto f = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(a);
        return f(instance, pCreateInfo, pAllocator, pCallback);
}

inline void vkDestroyDebugReportCallbackEXT(
        VkInstance instance,
        VkDebugReportCallbackEXT callback,
        const VkAllocationCallbacks* pAllocator)
{
        auto a = ns::vulkan::proc_addr(instance, "vkDestroyDebugReportCallbackEXT");
        auto f = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(a);
        f(instance, callback, pAllocator);
}

inline void vkDebugReportMessageEXT(
        VkInstance instance,
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char* pLayerPrefix,
        const char* pMessage)
{
        auto a = ns::vulkan::proc_addr(instance, "vkDebugReportMessageEXT");
        auto f = reinterpret_cast<PFN_vkDebugReportMessageEXT>(a);
        f(instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
}
