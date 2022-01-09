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

#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
struct DeviceProperties final
{
        VkPhysicalDeviceProperties properties_10;
        VkPhysicalDeviceVulkan11Properties properties_11;
        VkPhysicalDeviceVulkan12Properties properties_12;
};

struct DeviceFeatures final
{
        VkPhysicalDeviceFeatures features_10{};
        VkPhysicalDeviceVulkan11Features features_11{};
        VkPhysicalDeviceVulkan12Features features_12{};
};

struct DeviceInfo final
{
        std::unordered_set<std::string> extensions;
        DeviceProperties properties;
        DeviceFeatures features;
        std::vector<VkQueueFamilyProperties> queue_families;
};

std::vector<VkPhysicalDevice> find_physical_devices(VkInstance instance);

DeviceInfo find_physical_device_info(VkPhysicalDevice device);

std::vector<bool> find_queue_family_presentation_support(VkSurfaceKHR surface, VkPhysicalDevice device);

void add_device_features(VkPhysicalDeviceFeatures2* features_2, DeviceFeatures* features);

DeviceFeatures extract_device_features(const VkDeviceCreateInfo& create_info);
}
