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
namespace
{
template <typename T>
void set_instance(const VkInstance instance, const char* const name, T*& ptr)
{
        ptr = reinterpret_cast<T*>(vkGetInstanceProcAddr(instance, name));
}

template <typename T>
void set_device(const VkDevice device, const char* const name, T*& ptr)
{
        ptr = reinterpret_cast<T*>(vkGetDeviceProcAddr(device, name));
}

#define SET_INSTANCE(name) set_instance((instance), #name, (name));
#define SET_DEVICE(name) set_device((device), #name, (name));

template <typename T>
void reset(T*& ptr)
{
        ptr = nullptr;
}
}

PFN_vkVoidFunction instance_proc_addr(const VkInstance instance, const char* const name)
{
        ASSERT(instance != VK_NULL_HANDLE);
        const PFN_vkVoidFunction addr = vkGetInstanceProcAddr(instance, name);
        if (addr)
        {
                return addr;
        }
        error(std::string("Failed to find address of ") + name);
}

//

InstanceExtensionFunctions::InstanceExtensionFunctions(const VkInstance instance) : lock_(mutex_, std::try_to_lock)
{
        ASSERT(instance != VK_NULL_HANDLE);

        if (!lock_)
        {
                error("Vulkan instance extension function pointers are busy");
        }

        SET_INSTANCE(vkDestroySurfaceKHR)
        SET_INSTANCE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
        SET_INSTANCE(vkGetPhysicalDeviceSurfaceFormatsKHR)
        SET_INSTANCE(vkGetPhysicalDeviceSurfacePresentModesKHR)
        SET_INSTANCE(vkGetPhysicalDeviceSurfaceSupportKHR)
}

InstanceExtensionFunctions::~InstanceExtensionFunctions()
{
        reset(vkDestroySurfaceKHR);
        reset(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        reset(vkGetPhysicalDeviceSurfaceFormatsKHR);
        reset(vkGetPhysicalDeviceSurfacePresentModesKHR);
        reset(vkGetPhysicalDeviceSurfaceSupportKHR);
}

//

DeviceExtensionFunctions::DeviceExtensionFunctions(const VkDevice device) : lock_(mutex_, std::try_to_lock)
{
        ASSERT(device != VK_NULL_HANDLE);

        if (!lock_)
        {
                error("Vulkan device extension function pointers are busy");
        }

        SET_DEVICE(vkAcquireNextImageKHR)
        SET_DEVICE(vkCreateSwapchainKHR)
        SET_DEVICE(vkDestroySwapchainKHR)
        SET_DEVICE(vkGetSwapchainImagesKHR)
        SET_DEVICE(vkQueuePresentKHR)

        SET_DEVICE(vkCmdSetRayTracingPipelineStackSizeKHR)
        SET_DEVICE(vkCmdTraceRaysIndirectKHR)
        SET_DEVICE(vkCmdTraceRaysKHR)
        SET_DEVICE(vkCreateRayTracingPipelinesKHR)
        SET_DEVICE(vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)
        SET_DEVICE(vkGetRayTracingShaderGroupHandlesKHR)
        SET_DEVICE(vkGetRayTracingShaderGroupStackSizeKHR)

        SET_DEVICE(vkBuildAccelerationStructuresKHR)
        SET_DEVICE(vkCmdBuildAccelerationStructuresIndirectKHR)
        SET_DEVICE(vkCmdBuildAccelerationStructuresKHR)
        SET_DEVICE(vkCmdCopyAccelerationStructureKHR)
        SET_DEVICE(vkCmdCopyAccelerationStructureToMemoryKHR)
        SET_DEVICE(vkCmdCopyMemoryToAccelerationStructureKHR)
        SET_DEVICE(vkCmdWriteAccelerationStructuresPropertiesKHR)
        SET_DEVICE(vkCopyAccelerationStructureKHR)
        SET_DEVICE(vkCopyAccelerationStructureToMemoryKHR)
        SET_DEVICE(vkCopyMemoryToAccelerationStructureKHR)
        SET_DEVICE(vkCreateAccelerationStructureKHR)
        SET_DEVICE(vkDestroyAccelerationStructureKHR)
        SET_DEVICE(vkGetAccelerationStructureBuildSizesKHR)
        SET_DEVICE(vkGetAccelerationStructureDeviceAddressKHR)
        SET_DEVICE(vkGetDeviceAccelerationStructureCompatibilityKHR)
        SET_DEVICE(vkWriteAccelerationStructuresPropertiesKHR)
}

DeviceExtensionFunctions::~DeviceExtensionFunctions()
{
        reset(vkAcquireNextImageKHR);
        reset(vkCreateSwapchainKHR);
        reset(vkDestroySwapchainKHR);
        reset(vkGetSwapchainImagesKHR);
        reset(vkQueuePresentKHR);

        reset(vkCmdSetRayTracingPipelineStackSizeKHR);
        reset(vkCmdTraceRaysIndirectKHR);
        reset(vkCmdTraceRaysKHR);
        reset(vkCreateRayTracingPipelinesKHR);
        reset(vkGetRayTracingCaptureReplayShaderGroupHandlesKHR);
        reset(vkGetRayTracingShaderGroupHandlesKHR);
        reset(vkGetRayTracingShaderGroupStackSizeKHR);

        reset(vkBuildAccelerationStructuresKHR);
        reset(vkCmdBuildAccelerationStructuresIndirectKHR);
        reset(vkCmdBuildAccelerationStructuresKHR);
        reset(vkCmdCopyAccelerationStructureKHR);
        reset(vkCmdCopyAccelerationStructureToMemoryKHR);
        reset(vkCmdCopyMemoryToAccelerationStructureKHR);
        reset(vkCmdWriteAccelerationStructuresPropertiesKHR);
        reset(vkCopyAccelerationStructureKHR);
        reset(vkCopyAccelerationStructureToMemoryKHR);
        reset(vkCopyMemoryToAccelerationStructureKHR);
        reset(vkCreateAccelerationStructureKHR);
        reset(vkDestroyAccelerationStructureKHR);
        reset(vkGetAccelerationStructureBuildSizesKHR);
        reset(vkGetAccelerationStructureDeviceAddressKHR);
        reset(vkGetDeviceAccelerationStructureCompatibilityKHR);
        reset(vkWriteAccelerationStructuresPropertiesKHR);
}
}
