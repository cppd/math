/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <mutex>
#include <string>
#include <vulkan/vulkan_core.h>

namespace ns::vulkan
{
PFN_vkVoidFunction instance_proc_addr(const VkInstance instance, const char* const name)
{
        ASSERT(instance != VK_NULL_HANDLE);
        if (const PFN_vkVoidFunction addr = vkGetInstanceProcAddr(instance, name))
        {
                return addr;
        }
        error(std::string("Failed to find address of ") + name);
}

#define SET_INSTANCE(instance, name) \
        (name) = reinterpret_cast<decltype(name)>(vkGetInstanceProcAddr((instance), #name));

#define SET_DEVICE(device, name) (name) = reinterpret_cast<decltype(name)>(vkGetDeviceProcAddr((device), #name));

InstanceExtensionFunctions::InstanceExtensionFunctions(const VkInstance instance)
        : lock_(mutex_, std::try_to_lock)
{
        ASSERT(instance != VK_NULL_HANDLE);

        if (!lock_)
        {
                error("Vulkan instance extension function pointers are busy");
        }

        SET_INSTANCE(instance, vkDestroySurfaceKHR)
        SET_INSTANCE(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
        SET_INSTANCE(instance, vkGetPhysicalDeviceSurfaceFormatsKHR)
        SET_INSTANCE(instance, vkGetPhysicalDeviceSurfacePresentModesKHR)
        SET_INSTANCE(instance, vkGetPhysicalDeviceSurfaceSupportKHR)

        SET_INSTANCE(instance, vkCmdBeginDebugUtilsLabelEXT)
        SET_INSTANCE(instance, vkCmdEndDebugUtilsLabelEXT)
        SET_INSTANCE(instance, vkCmdInsertDebugUtilsLabelEXT)
        SET_INSTANCE(instance, vkCreateDebugUtilsMessengerEXT)
        SET_INSTANCE(instance, vkDestroyDebugUtilsMessengerEXT)
        SET_INSTANCE(instance, vkQueueBeginDebugUtilsLabelEXT)
        SET_INSTANCE(instance, vkQueueEndDebugUtilsLabelEXT)
        SET_INSTANCE(instance, vkQueueInsertDebugUtilsLabelEXT)
        SET_INSTANCE(instance, vkSetDebugUtilsObjectNameEXT)
        SET_INSTANCE(instance, vkSetDebugUtilsObjectTagEXT)
        SET_INSTANCE(instance, vkSubmitDebugUtilsMessageEXT)
}

InstanceExtensionFunctions::~InstanceExtensionFunctions()
{
        vkDestroySurfaceKHR = nullptr;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
        vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
        vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
        vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;

        vkCmdBeginDebugUtilsLabelEXT = nullptr;
        vkCmdEndDebugUtilsLabelEXT = nullptr;
        vkCmdInsertDebugUtilsLabelEXT = nullptr;
        vkCreateDebugUtilsMessengerEXT = nullptr;
        vkDestroyDebugUtilsMessengerEXT = nullptr;
        vkQueueBeginDebugUtilsLabelEXT = nullptr;
        vkQueueEndDebugUtilsLabelEXT = nullptr;
        vkQueueInsertDebugUtilsLabelEXT = nullptr;
        vkSetDebugUtilsObjectNameEXT = nullptr;
        vkSetDebugUtilsObjectTagEXT = nullptr;
        vkSubmitDebugUtilsMessageEXT = nullptr;
}

DeviceExtensionFunctions::DeviceExtensionFunctions(const VkDevice device)
        : lock_(mutex_, std::try_to_lock)
{
        ASSERT(device != VK_NULL_HANDLE);

        if (!lock_)
        {
                error("Vulkan device extension function pointers are busy");
        }

        SET_DEVICE(device, vkAcquireNextImageKHR)
        SET_DEVICE(device, vkCreateSwapchainKHR)
        SET_DEVICE(device, vkDestroySwapchainKHR)
        SET_DEVICE(device, vkGetSwapchainImagesKHR)
        SET_DEVICE(device, vkQueuePresentKHR)

        SET_DEVICE(device, vkCmdSetRayTracingPipelineStackSizeKHR)
        SET_DEVICE(device, vkCmdTraceRaysIndirectKHR)
        SET_DEVICE(device, vkCmdTraceRaysKHR)
        SET_DEVICE(device, vkCreateRayTracingPipelinesKHR)
        SET_DEVICE(device, vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)
        SET_DEVICE(device, vkGetRayTracingShaderGroupHandlesKHR)
        SET_DEVICE(device, vkGetRayTracingShaderGroupStackSizeKHR)

        SET_DEVICE(device, vkBuildAccelerationStructuresKHR)
        SET_DEVICE(device, vkCmdBuildAccelerationStructuresIndirectKHR)
        SET_DEVICE(device, vkCmdBuildAccelerationStructuresKHR)
        SET_DEVICE(device, vkCmdCopyAccelerationStructureKHR)
        SET_DEVICE(device, vkCmdCopyAccelerationStructureToMemoryKHR)
        SET_DEVICE(device, vkCmdCopyMemoryToAccelerationStructureKHR)
        SET_DEVICE(device, vkCmdWriteAccelerationStructuresPropertiesKHR)
        SET_DEVICE(device, vkCopyAccelerationStructureKHR)
        SET_DEVICE(device, vkCopyAccelerationStructureToMemoryKHR)
        SET_DEVICE(device, vkCopyMemoryToAccelerationStructureKHR)
        SET_DEVICE(device, vkCreateAccelerationStructureKHR)
        SET_DEVICE(device, vkDestroyAccelerationStructureKHR)
        SET_DEVICE(device, vkGetAccelerationStructureBuildSizesKHR)
        SET_DEVICE(device, vkGetAccelerationStructureDeviceAddressKHR)
        SET_DEVICE(device, vkGetDeviceAccelerationStructureCompatibilityKHR)
        SET_DEVICE(device, vkWriteAccelerationStructuresPropertiesKHR)
}

DeviceExtensionFunctions::~DeviceExtensionFunctions()
{
        vkAcquireNextImageKHR = nullptr;
        vkCreateSwapchainKHR = nullptr;
        vkDestroySwapchainKHR = nullptr;
        vkGetSwapchainImagesKHR = nullptr;
        vkQueuePresentKHR = nullptr;

        vkCmdSetRayTracingPipelineStackSizeKHR = nullptr;
        vkCmdTraceRaysIndirectKHR = nullptr;
        vkCmdTraceRaysKHR = nullptr;
        vkCreateRayTracingPipelinesKHR = nullptr;
        vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = nullptr;
        vkGetRayTracingShaderGroupHandlesKHR = nullptr;
        vkGetRayTracingShaderGroupStackSizeKHR = nullptr;

        vkBuildAccelerationStructuresKHR = nullptr;
        vkCmdBuildAccelerationStructuresIndirectKHR = nullptr;
        vkCmdBuildAccelerationStructuresKHR = nullptr;
        vkCmdCopyAccelerationStructureKHR = nullptr;
        vkCmdCopyAccelerationStructureToMemoryKHR = nullptr;
        vkCmdCopyMemoryToAccelerationStructureKHR = nullptr;
        vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;
        vkCopyAccelerationStructureKHR = nullptr;
        vkCopyAccelerationStructureToMemoryKHR = nullptr;
        vkCopyMemoryToAccelerationStructureKHR = nullptr;
        vkCreateAccelerationStructureKHR = nullptr;
        vkDestroyAccelerationStructureKHR = nullptr;
        vkGetAccelerationStructureBuildSizesKHR = nullptr;
        vkGetAccelerationStructureDeviceAddressKHR = nullptr;
        vkGetDeviceAccelerationStructureCompatibilityKHR = nullptr;
        vkWriteAccelerationStructuresPropertiesKHR = nullptr;
}
}
