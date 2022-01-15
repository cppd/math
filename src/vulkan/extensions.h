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

#pragma once

#include <mutex>
#include <vulkan/vulkan.h>

namespace ns
{
namespace vulkan
{
PFN_vkVoidFunction proc_addr(VkInstance instance, const char* name);

class InstanceExtensionFunctions final
{
        inline static std::mutex mutex_;
        std::unique_lock<std::mutex> lock_;

public:
        explicit InstanceExtensionFunctions(VkInstance instance);
        ~InstanceExtensionFunctions();

        InstanceExtensionFunctions(const InstanceExtensionFunctions&) = delete;
        InstanceExtensionFunctions(InstanceExtensionFunctions&&) = delete;
        InstanceExtensionFunctions& operator=(const InstanceExtensionFunctions&) = delete;
        InstanceExtensionFunctions& operator=(InstanceExtensionFunctions&&) = delete;
};

class DeviceExtensionFunctions final
{
        inline static std::mutex mutex_;
        std::unique_lock<std::mutex> lock_;

public:
        explicit DeviceExtensionFunctions(VkDevice device);
        ~DeviceExtensionFunctions();

        DeviceExtensionFunctions(const DeviceExtensionFunctions&) = delete;
        DeviceExtensionFunctions(DeviceExtensionFunctions&&) = delete;
        DeviceExtensionFunctions& operator=(const DeviceExtensionFunctions&) = delete;
        DeviceExtensionFunctions& operator=(DeviceExtensionFunctions&&) = delete;
};
}

#define VULKAN_EXTENSION_FUNCTION(name)                            \
        template <typename... T>                                   \
        decltype(auto) name(const VkInstance instance, T&&... v)   \
        {                                                          \
                const auto p = vulkan::proc_addr(instance, #name); \
                const auto f = reinterpret_cast<PFN_##name>(p);    \
                return f(instance, std::forward<T>(v)...);         \
        }

VULKAN_EXTENSION_FUNCTION(vkCreateDebugUtilsMessengerEXT)
VULKAN_EXTENSION_FUNCTION(vkDestroyDebugUtilsMessengerEXT)

//

inline PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
inline PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
inline PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
inline PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
inline PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;

//

inline PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
inline PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
inline PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
inline PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
inline PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;
}
