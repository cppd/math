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

#include "extensions.h"

#include <src/com/error.h>

namespace ns::vulkan
{
PFN_vkVoidFunction proc_addr(const VkInstance instance, const char* const name)
{
        ASSERT(instance != VK_NULL_HANDLE);
        const PFN_vkVoidFunction addr = vkGetInstanceProcAddr(instance, name);
        if (addr)
        {
                return addr;
        }
        error(std::string("Failed to find address of ") + name);
}

#define SET_INSTANCE_ADDRESS(name)                                                               \
        do                                                                                       \
        {                                                                                        \
                (name) = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr((instance), #name)); \
        } while (false)

#define SET_DEVICE_ADDRESS(name)                                                             \
        do                                                                                   \
        {                                                                                    \
                (name) = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr((device), #name)); \
        } while (false)

//

InstanceExtensions::InstanceExtensions(const VkInstance instance) : lock_(mutex_, std::try_to_lock)
{
        ASSERT(instance != VK_NULL_HANDLE);

        if (!lock_)
        {
                error("Vulkan instance extension function pointers are busy");
        }

        SET_INSTANCE_ADDRESS(vkDestroySurfaceKHR);
        SET_INSTANCE_ADDRESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        SET_INSTANCE_ADDRESS(vkGetPhysicalDeviceSurfaceFormatsKHR);
        SET_INSTANCE_ADDRESS(vkGetPhysicalDeviceSurfacePresentModesKHR);
        SET_INSTANCE_ADDRESS(vkGetPhysicalDeviceSurfaceSupportKHR);
}

InstanceExtensions::~InstanceExtensions()
{
        vkDestroySurfaceKHR = nullptr;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
        vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
        vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
        vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
}

//

DeviceExtensions::DeviceExtensions(const VkDevice device) : lock_(mutex_, std::try_to_lock)
{
        ASSERT(device != VK_NULL_HANDLE);

        if (!lock_)
        {
                error("Vulkan device extension function pointers are busy");
        }

        SET_DEVICE_ADDRESS(vkAcquireNextImageKHR);
        SET_DEVICE_ADDRESS(vkCreateSwapchainKHR);
        SET_DEVICE_ADDRESS(vkDestroySwapchainKHR);
        SET_DEVICE_ADDRESS(vkGetSwapchainImagesKHR);
        SET_DEVICE_ADDRESS(vkQueuePresentKHR);
}

DeviceExtensions::~DeviceExtensions()
{
        vkAcquireNextImageKHR = nullptr;
        vkCreateSwapchainKHR = nullptr;
        vkDestroySwapchainKHR = nullptr;
        vkGetSwapchainImagesKHR = nullptr;
        vkQueuePresentKHR = nullptr;
}
}
