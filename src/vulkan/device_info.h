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

#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
struct DeviceFeatures final
{
        VkPhysicalDeviceFeatures features_10{};
        VkPhysicalDeviceVulkan11Features features_11{};
        VkPhysicalDeviceVulkan12Features features_12{};
};

struct DeviceProperties final
{
        VkPhysicalDeviceProperties properties_10;
        VkPhysicalDeviceVulkan11Properties properties_11;
        VkPhysicalDeviceVulkan12Properties properties_12;
};

std::vector<bool> find_presentation_support(
        VkSurfaceKHR surface,
        VkPhysicalDevice device,
        const std::vector<VkQueueFamilyProperties>& queue_families);

std::vector<VkQueueFamilyProperties> find_queue_families(VkPhysicalDevice device);

std::unordered_set<std::string> find_device_extensions(VkPhysicalDevice device);

std::vector<VkPhysicalDevice> find_physical_devices(VkInstance instance);

DeviceProperties find_physical_device_properties(VkPhysicalDevice device);

DeviceFeatures find_physical_device_features(VkPhysicalDevice device);

DeviceFeatures extract_device_features(const VkDeviceCreateInfo& create_info);
}
