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

#pragma once

#include <mutex>
#include <vulkan/vulkan_core.h>

namespace ns
{
namespace vulkan
{
PFN_vkVoidFunction instance_proc_addr(VkInstance instance, const char* name);

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

#define VULKAN_INSTANCE_PROC_ADDR(instance, name) \
        reinterpret_cast<PFN_##name>(::ns::vulkan::instance_proc_addr((instance), #name))

//

#define VULKAN_EXTENSION_POINTER(name) inline PFN_##name name = nullptr;

//

// VK_KHR_surface
VULKAN_EXTENSION_POINTER(vkDestroySurfaceKHR)
VULKAN_EXTENSION_POINTER(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
VULKAN_EXTENSION_POINTER(vkGetPhysicalDeviceSurfaceFormatsKHR)
VULKAN_EXTENSION_POINTER(vkGetPhysicalDeviceSurfacePresentModesKHR)
VULKAN_EXTENSION_POINTER(vkGetPhysicalDeviceSurfaceSupportKHR)

// VK_EXT_debug_utils
VULKAN_EXTENSION_POINTER(vkCmdBeginDebugUtilsLabelEXT)
VULKAN_EXTENSION_POINTER(vkCmdEndDebugUtilsLabelEXT)
VULKAN_EXTENSION_POINTER(vkCmdInsertDebugUtilsLabelEXT)
VULKAN_EXTENSION_POINTER(vkCreateDebugUtilsMessengerEXT)
VULKAN_EXTENSION_POINTER(vkDestroyDebugUtilsMessengerEXT)
VULKAN_EXTENSION_POINTER(vkQueueBeginDebugUtilsLabelEXT)
VULKAN_EXTENSION_POINTER(vkQueueEndDebugUtilsLabelEXT)
VULKAN_EXTENSION_POINTER(vkQueueInsertDebugUtilsLabelEXT)
VULKAN_EXTENSION_POINTER(vkSetDebugUtilsObjectNameEXT)
VULKAN_EXTENSION_POINTER(vkSetDebugUtilsObjectTagEXT)
VULKAN_EXTENSION_POINTER(vkSubmitDebugUtilsMessageEXT)

//

// VK_KHR_swapchain
VULKAN_EXTENSION_POINTER(vkAcquireNextImageKHR)
VULKAN_EXTENSION_POINTER(vkCreateSwapchainKHR)
VULKAN_EXTENSION_POINTER(vkDestroySwapchainKHR)
VULKAN_EXTENSION_POINTER(vkGetSwapchainImagesKHR)
VULKAN_EXTENSION_POINTER(vkQueuePresentKHR)

// VK_KHR_ray_tracing_pipeline
VULKAN_EXTENSION_POINTER(vkCmdSetRayTracingPipelineStackSizeKHR)
VULKAN_EXTENSION_POINTER(vkCmdTraceRaysIndirectKHR)
VULKAN_EXTENSION_POINTER(vkCmdTraceRaysKHR)
VULKAN_EXTENSION_POINTER(vkCreateRayTracingPipelinesKHR)
VULKAN_EXTENSION_POINTER(vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)
VULKAN_EXTENSION_POINTER(vkGetRayTracingShaderGroupHandlesKHR)
VULKAN_EXTENSION_POINTER(vkGetRayTracingShaderGroupStackSizeKHR)

// VK_KHR_acceleration_structure
VULKAN_EXTENSION_POINTER(vkBuildAccelerationStructuresKHR)
VULKAN_EXTENSION_POINTER(vkCmdBuildAccelerationStructuresIndirectKHR)
VULKAN_EXTENSION_POINTER(vkCmdBuildAccelerationStructuresKHR)
VULKAN_EXTENSION_POINTER(vkCmdCopyAccelerationStructureKHR)
VULKAN_EXTENSION_POINTER(vkCmdCopyAccelerationStructureToMemoryKHR)
VULKAN_EXTENSION_POINTER(vkCmdCopyMemoryToAccelerationStructureKHR)
VULKAN_EXTENSION_POINTER(vkCmdWriteAccelerationStructuresPropertiesKHR)
VULKAN_EXTENSION_POINTER(vkCopyAccelerationStructureKHR)
VULKAN_EXTENSION_POINTER(vkCopyAccelerationStructureToMemoryKHR)
VULKAN_EXTENSION_POINTER(vkCopyMemoryToAccelerationStructureKHR)
VULKAN_EXTENSION_POINTER(vkCreateAccelerationStructureKHR)
VULKAN_EXTENSION_POINTER(vkDestroyAccelerationStructureKHR)
VULKAN_EXTENSION_POINTER(vkGetAccelerationStructureBuildSizesKHR)
VULKAN_EXTENSION_POINTER(vkGetAccelerationStructureDeviceAddressKHR)
VULKAN_EXTENSION_POINTER(vkGetDeviceAccelerationStructureCompatibilityKHR)
VULKAN_EXTENSION_POINTER(vkWriteAccelerationStructuresPropertiesKHR)
}
