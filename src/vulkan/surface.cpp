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

#include "surface.h"

#include "error.h"
#include "extensions.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::vulkan
{
VkSurfaceCapabilitiesKHR find_surface_capabilities(const VkPhysicalDevice device, const VkSurfaceKHR surface)
{
        VkSurfaceCapabilitiesKHR surface_capabilities;
        VULKAN_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities));
        return surface_capabilities;
}

std::vector<VkSurfaceFormatKHR> find_surface_formats(const VkPhysicalDevice device, const VkSurfaceKHR surface)
{
        std::uint32_t count;
        VULKAN_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr));

        std::vector<VkSurfaceFormatKHR> formats(count);

        if (formats.empty())
        {
                return formats;
        }

        VULKAN_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data()));
        return formats;
}

std::vector<VkPresentModeKHR> find_present_modes(const VkPhysicalDevice device, const VkSurfaceKHR surface)
{
        std::uint32_t count;
        VULKAN_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr));

        std::vector<VkPresentModeKHR> modes(count);

        if (modes.empty())
        {
                return modes;
        }

        VULKAN_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes.data()));
        return modes;
}

bool surface_suitable(const VkPhysicalDevice device, const VkSurfaceKHR surface)
{
        VkSurfaceCapabilitiesKHR surface_capabilities;
        VULKAN_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities));

        return !find_surface_formats(device, surface).empty() && !find_present_modes(device, surface).empty();
}

VkExtent2D choose_surface_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
        constexpr std::uint32_t SPECIAL_VALUE = 0xffff'ffff;

        if (capabilities.currentExtent.width == SPECIAL_VALUE && capabilities.currentExtent.height == SPECIAL_VALUE)
        {
                return capabilities.maxImageExtent;
        }

        return capabilities.currentExtent;
}
}
