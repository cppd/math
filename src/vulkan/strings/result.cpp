/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <vulkan/vulkan_core.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan::strings
{
std::string result_to_string(const VkResult result)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (result)
        {
                CASE(VK_SUCCESS)
                CASE(VK_NOT_READY)
                CASE(VK_TIMEOUT)
                CASE(VK_EVENT_SET)
                CASE(VK_EVENT_RESET)
                CASE(VK_INCOMPLETE)
                CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
                CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
                CASE(VK_ERROR_INITIALIZATION_FAILED)
                CASE(VK_ERROR_DEVICE_LOST)
                CASE(VK_ERROR_MEMORY_MAP_FAILED)
                CASE(VK_ERROR_LAYER_NOT_PRESENT)
                CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
                CASE(VK_ERROR_FEATURE_NOT_PRESENT)
                CASE(VK_ERROR_INCOMPATIBLE_DRIVER)
                CASE(VK_ERROR_TOO_MANY_OBJECTS)
                CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
                CASE(VK_ERROR_FRAGMENTED_POOL)
                CASE(VK_ERROR_UNKNOWN)
                CASE(VK_ERROR_OUT_OF_POOL_MEMORY)
                CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE)
                CASE(VK_ERROR_FRAGMENTATION)
                CASE(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
                CASE(VK_PIPELINE_COMPILE_REQUIRED)
                CASE(VK_ERROR_NOT_PERMITTED)
                CASE(VK_ERROR_SURFACE_LOST_KHR)
                CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
                CASE(VK_SUBOPTIMAL_KHR)
                CASE(VK_ERROR_OUT_OF_DATE_KHR)
                CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
                CASE(VK_ERROR_VALIDATION_FAILED_EXT)
                CASE(VK_ERROR_INVALID_SHADER_NV)
                CASE(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
                CASE(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
                CASE(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
                CASE(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
                CASE(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
                CASE(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
                CASE(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
                CASE(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
                CASE(VK_THREAD_IDLE_KHR)
                CASE(VK_THREAD_DONE_KHR)
                CASE(VK_OPERATION_DEFERRED_KHR)
                CASE(VK_OPERATION_NOT_DEFERRED_KHR)
                CASE(VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)
                CASE(VK_ERROR_COMPRESSION_EXHAUSTED_EXT)
                CASE(VK_INCOMPATIBLE_SHADER_BINARY_EXT)
                CASE(VK_PIPELINE_BINARY_MISSING_KHR)
                CASE(VK_ERROR_NOT_ENOUGH_SPACE_KHR)
        }
#pragma GCC diagnostic pop

        return "Unknown VkResult " + to_string(enum_to_int(result));
}
}
