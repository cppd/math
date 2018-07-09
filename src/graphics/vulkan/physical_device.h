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

#pragma once

#if defined(VULKAN_FOUND)

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan
{
struct SwapChainDetails
{
        VkSurfaceCapabilitiesKHR surface_capabilities;
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR> present_modes;
};

struct PhysicalDevice
{
        const VkPhysicalDevice physical_device;
        const uint32_t graphics_family_index;
        const uint32_t compute_family_index;
        const uint32_t presentation_family_index;
};

bool find_swap_chain_details(VkSurfaceKHR surface, VkPhysicalDevice device, SwapChainDetails* swap_chain_details);

vulkan::PhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major,
                                            int api_version_minor, const std::vector<std::string>& required_extensions);
}

#endif
