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

#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
bool find_surface_details(
        VkSurfaceKHR surface,
        VkPhysicalDevice device,
        VkSurfaceCapabilitiesKHR* surface_capabilities,
        std::vector<VkSurfaceFormatKHR>* surface_formats,
        std::vector<VkPresentModeKHR>* present_modes);

bool surface_suitable(VkSurfaceKHR surface, VkPhysicalDevice device);
}
