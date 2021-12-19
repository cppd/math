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

#include "result.h"

#include <src/com/enum.h>
#include <src/com/print.h>

namespace ns::vulkan
{
std::array<std::string, 2> result_to_strings(const VkResult code)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (code)
        {
        case VK_SUCCESS:
                return {"VK_SUCCESS", "Command successfully completed"};
        case VK_NOT_READY:
                return {"VK_NOT_READY", "A fence or query has not yet completed"};
        case VK_TIMEOUT:
                return {"VK_TIMEOUT", "A wait operation has not completed in the specified time"};
        case VK_EVENT_SET:
                return {"VK_EVENT_SET", "An event is signaled"};
        case VK_EVENT_RESET:
                return {"VK_EVENT_RESET", "An event is unsignaled"};
        case VK_INCOMPLETE:
                return {"VK_INCOMPLETE", "A return array was too small for the result"};
        case VK_ERROR_OUT_OF_HOST_MEMORY:
                return {"VK_ERROR_OUT_OF_HOST_MEMORY", "A host memory allocation has failed"};
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return {"VK_ERROR_OUT_OF_DEVICE_MEMORY", "A device memory allocation has failed"};
        case VK_ERROR_INITIALIZATION_FAILED:
                return {"VK_ERROR_INITIALIZATION_FAILED",
                        "Initialization of an object could not be completed for implementation-specific reasons"};
        case VK_ERROR_DEVICE_LOST:
                return {"VK_ERROR_DEVICE_LOST", "The logical or physical device has been lost"};
        case VK_ERROR_MEMORY_MAP_FAILED:
                return {"VK_ERROR_MEMORY_MAP_FAILED", "Mapping of a memory object has failed"};
        case VK_ERROR_LAYER_NOT_PRESENT:
                return {"VK_ERROR_LAYER_NOT_PRESENT", "A requested layer is not present or could not be loaded"};
        case VK_ERROR_EXTENSION_NOT_PRESENT:
                return {"VK_ERROR_EXTENSION_NOT_PRESENT", "A requested extension is not supported"};
        case VK_ERROR_FEATURE_NOT_PRESENT:
                return {"VK_ERROR_FEATURE_NOT_PRESENT", "A requested feature is not supported"};
        case VK_ERROR_INCOMPATIBLE_DRIVER:
                return {"VK_ERROR_INCOMPATIBLE_DRIVER",
                        "The requested version of Vulkan is not supported by the driver or is otherwise"
                        " incompatible for implementation-specific reasons"};
        case VK_ERROR_TOO_MANY_OBJECTS:
                return {"VK_ERROR_TOO_MANY_OBJECTS", "Too many objects of the type have already been created"};
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return {"VK_ERROR_FORMAT_NOT_SUPPORTED", "A requested format is not supported on this device"};
        case VK_ERROR_FRAGMENTED_POOL:
                return {"VK_ERROR_FRAGMENTED_POOL",
                        "A pool allocation has failed due to fragmentation of the pool’s memory"};
        case VK_ERROR_UNKNOWN:
                return {"VK_ERROR_UNKNOWN", "An unknown error has occurred."};
        case VK_ERROR_OUT_OF_POOL_MEMORY:
                return {"VK_ERROR_OUT_OF_POOL_MEMORY", "A pool memory allocation has failed"};
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                return {"VK_ERROR_INVALID_EXTERNAL_HANDLE",
                        "An external handle is not a valid handle of the specified type"};
        case VK_ERROR_FRAGMENTATION:
                return {"VK_ERROR_FRAGMENTATION", "A descriptor pool creation has failed due to fragmentation"};
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
                return {"VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS",
                        "A buffer creation or memory allocation failed because"
                        " the requested address is not available. A shader group handle assignment failed"
                        " because the requested shader group handle information is no longer valid."};
        case VK_ERROR_SURFACE_LOST_KHR:
                return {"VK_ERROR_SURFACE_LOST_KHR", "A surface is no longer available"};
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return {"VK_ERROR_NATIVE_WINDOW_IN_USE_KHR",
                        "The requested window is already in use by Vulkan or another API in a manner which"
                        " prevents it from being used again"};
        case VK_SUBOPTIMAL_KHR:
                return {"VK_SUBOPTIMAL_KHR",
                        "A swapchain no longer matches the surface properties exactly, but can still be used"
                        " to present to the surface successfully"};
        case VK_ERROR_OUT_OF_DATE_KHR:
                return {"VK_ERROR_OUT_OF_DATE_KHR",
                        "A surface has changed in such a way that it is no longer compatible"
                        " with the swapchain, and further presentation requests using"
                        " the swapchain will fail"};
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                return {"VK_ERROR_INCOMPATIBLE_DISPLAY_KHR",
                        "The display used by a swapchain does not use the same presentable image"
                        " layout, or is incompatible in a way that prevents sharing an image"};
        case VK_ERROR_VALIDATION_FAILED_EXT:
                return {"VK_ERROR_VALIDATION_FAILED_EXT", ""};
        case VK_ERROR_INVALID_SHADER_NV:
                return {"VK_ERROR_INVALID_SHADER_NV", "One or more shaders failed to compile or link"};
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
                return {"VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT", ""};
        case VK_ERROR_NOT_PERMITTED_EXT:
                return {"VK_ERROR_NOT_PERMITTED_EXT", ""};
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
                return {"VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT",
                        "An operation on a swapchain created with"
                        " VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have"
                        " exlusive full-screen access"};
        case VK_THREAD_IDLE_KHR:
                return {"VK_THREAD_IDLE_KHR",
                        "A deferred operation is not complete but there is currently no work for this"
                        " thread to do at the time of this call"};
        case VK_THREAD_DONE_KHR:
                return {"VK_THREAD_DONE_KHR",
                        "A deferred operation is not complete but there is no work remaining to"
                        " assign to additional threads"};
        case VK_OPERATION_DEFERRED_KHR:
                return {"VK_OPERATION_DEFERRED_KHR",
                        "A deferred operation was requested and at least some of the work was deferred"};
        case VK_OPERATION_NOT_DEFERRED_KHR:
                return {"VK_OPERATION_NOT_DEFERRED_KHR",
                        "A deferred operation was requested and no operations were deferred"};
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
                return {"VK_PIPELINE_COMPILE_REQUIRED_EXT",
                        "A requested pipeline creation would have required compilation,"
                        " but the application requested compilation to not be performed."};
        }
#pragma GCC diagnostic pop

        return {"Unknown VkResult " + to_string(enum_to_int(code)), ""};
}
}
