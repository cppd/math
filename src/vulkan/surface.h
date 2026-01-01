/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::vulkan
{
[[nodiscard]] VkSurfaceCapabilitiesKHR find_surface_capabilities(VkPhysicalDevice device, VkSurfaceKHR surface);

[[nodiscard]] std::vector<VkSurfaceFormatKHR> find_surface_formats(VkPhysicalDevice device, VkSurfaceKHR surface);

[[nodiscard]] std::vector<VkPresentModeKHR> find_present_modes(VkPhysicalDevice device, VkSurfaceKHR surface);

[[nodiscard]] bool surface_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);

[[nodiscard]] VkExtent2D choose_surface_extent(const VkSurfaceCapabilitiesKHR& capabilities);
}
