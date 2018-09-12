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

#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan
{
std::unordered_set<std::string> supported_instance_extensions();
std::unordered_set<std::string> supported_physical_device_extensions(VkPhysicalDevice physical_device);
std::unordered_set<std::string> supported_validation_layers();

uint32_t supported_instance_api_version();

std::vector<VkPhysicalDevice> physical_devices(VkInstance instance);
std::vector<VkQueueFamilyProperties> queue_families(VkPhysicalDevice device);

void check_instance_extension_support(const std::vector<std::string>& required_extensions);
void check_validation_layer_support(const std::vector<std::string>& required_layers);
void check_api_version(uint32_t required_api_version);

bool device_supports_extensions(VkPhysicalDevice physical_device, const std::vector<std::string>& extensions);

std::vector<VkSurfaceFormatKHR> surface_formats(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
std::vector<VkPresentModeKHR> present_modes(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
std::vector<VkImage> swap_chain_images(VkDevice device, VkSwapchainKHR swap_chain);

VkFormat find_supported_format(VkPhysicalDevice physical_device, const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);

std::string overview();
std::string overview_physical_devices(VkInstance instance);
}
