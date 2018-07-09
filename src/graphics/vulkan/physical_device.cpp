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

#if defined(VULKAN_FOUND)

#include "physical_device.h"

#include "common.h"
#include "query.h"

#include "com/error.h"

namespace
{
bool find_family_indices(VkSurfaceKHR surface, VkPhysicalDevice device, uint32_t* graphics_family_index,
                         uint32_t* compute_family_index, uint32_t* presentation_family_index)
{
        uint32_t index = 0;

        uint32_t graphics = 0;
        uint32_t compute = 0;
        uint32_t presentation = 0;

        bool graphics_found = false;
        bool compute_found = false;
        bool presentation_found = false;

        for (const VkQueueFamilyProperties& p : vulkan::queue_families(device))
        {
                if (p.queueCount < 1)
                {
                        continue;
                }

                if (!graphics_found && (p.queueFlags & VK_QUEUE_GRAPHICS_BIT))
                {
                        graphics_found = true;
                        graphics = index;
                }

                if (!compute_found && (p.queueFlags & VK_QUEUE_COMPUTE_BIT))
                {
                        compute_found = true;
                        compute = index;
                }

                if (!presentation_found)
                {
                        VkBool32 presentation_support;

                        VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentation_support);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceSupportKHR", result);
                        }

                        if (presentation_support == VK_TRUE)
                        {
                                presentation_found = true;
                                presentation = index;
                        }
                }

                if (graphics_found && compute_found && presentation_found)
                {
                        *graphics_family_index = graphics;
                        *compute_family_index = compute;
                        *presentation_family_index = presentation;

                        return true;
                }

                ++index;
        }

        return false;
}
}

namespace vulkan
{
bool find_swap_chain_details(VkSurfaceKHR surface, VkPhysicalDevice device, SwapChainDetails* swap_chain_details)
{
        VkSurfaceCapabilitiesKHR surface_capabilities;

        VkResult result;
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", result);
        }

        std::vector<VkSurfaceFormatKHR> surface_formats = vulkan::surface_formats(device, surface);
        if (surface_formats.empty())
        {
                return false;
        }

        std::vector<VkPresentModeKHR> present_modes = vulkan::present_modes(device, surface);
        if (present_modes.empty())
        {
                return false;
        }

        if (swap_chain_details)
        {
                swap_chain_details->surface_capabilities = surface_capabilities;
                swap_chain_details->surface_formats = surface_formats;
                swap_chain_details->present_modes = present_modes;
        }

        return true;
}

vulkan::PhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major,
                                            int api_version_minor, const std::vector<std::string>& required_extensions)
{
        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        for (const VkPhysicalDevice& device : vulkan::physical_devices(instance))
        {
                ASSERT(device != VK_NULL_HANDLE);

                VkPhysicalDeviceProperties properties;
                VkPhysicalDeviceFeatures features;
                vkGetPhysicalDeviceProperties(device, &properties);
                vkGetPhysicalDeviceFeatures(device, &features);

                if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU)
                {
                        continue;
                }

                if (!features.geometryShader)
                {
                        continue;
                }

                if (!features.tessellationShader)
                {
                        continue;
                }

                if (required_api_version > properties.apiVersion)
                {
                        continue;
                }

                if (!vulkan::device_supports_extensions(device, required_extensions))
                {
                        continue;
                }

                uint32_t graphics_family_index;
                uint32_t compute_family_index;
                uint32_t presentation_family_index;
                if (!find_family_indices(surface, device, &graphics_family_index, &compute_family_index,
                                         &presentation_family_index))
                {
                        continue;
                }

                if (!find_swap_chain_details(surface, device, nullptr))
                {
                        continue;
                }

                return {device, graphics_family_index, compute_family_index, presentation_family_index};
        }

        error("Failed to find a suitable Vulkan physical device");
}
}

#endif
