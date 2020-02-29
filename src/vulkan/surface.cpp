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

#include "surface.h"

#include "error.h"

#include <src/com/error.h>

namespace
{
std::vector<VkSurfaceFormatKHR> find_surface_formats(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
        uint32_t format_count;
        VkResult result;

        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceFormatsKHR", result);
        }

        if (format_count < 1)
        {
                return {};
        }

        std::vector<VkSurfaceFormatKHR> formats(format_count);

        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceFormatsKHR", result);
        }

        return formats;
}

std::vector<VkPresentModeKHR> find_present_modes(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
        uint32_t mode_count;
        VkResult result;

        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfacePresentModesKHR", result);
        }

        if (mode_count < 1)
        {
                return {};
        }

        std::vector<VkPresentModeKHR> modes(mode_count);

        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, modes.data());
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfacePresentModesKHR", result);
        }

        return modes;
}
}

namespace vulkan
{
bool find_surface_details(
        VkSurfaceKHR surface,
        VkPhysicalDevice device,
        VkSurfaceCapabilitiesKHR* surface_capabilities,
        std::vector<VkSurfaceFormatKHR>* surface_formats,
        std::vector<VkPresentModeKHR>* present_modes)
{
        ASSERT(surface_capabilities && surface_formats && present_modes);

        VkResult result;
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, surface_capabilities);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", result);
        }

        *surface_formats = find_surface_formats(device, surface);
        if (surface_formats->empty())
        {
                return false;
        }

        *present_modes = find_present_modes(device, surface);

        return !present_modes->empty();
}

bool surface_suitable(VkSurfaceKHR surface, VkPhysicalDevice physical_device)
{
        VkSurfaceCapabilitiesKHR surface_capabilities;
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR> present_modes;

        return find_surface_details(surface, physical_device, &surface_capabilities, &surface_formats, &present_modes);
}
}
