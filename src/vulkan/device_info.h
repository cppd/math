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

#include <optional>
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
        std::optional<VkPhysicalDeviceAccelerationStructurePropertiesKHR> acceleration_structure;
        std::optional<VkPhysicalDeviceRayTracingPipelinePropertiesKHR> ray_tracing_pipeline;
};

struct DeviceFeatures final
{
        VkPhysicalDeviceFeatures features_10{};
        VkPhysicalDeviceVulkan11Features features_11{};
        VkPhysicalDeviceVulkan12Features features_12{};
        std::optional<VkPhysicalDeviceAccelerationStructureFeaturesKHR> acceleration_structure;
        std::optional<VkPhysicalDeviceRayTracingPipelineFeaturesKHR> ray_tracing_pipeline;
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

void make_device_features(
        const DeviceFeatures& features,
        VkPhysicalDeviceFeatures2* features_2,
        DeviceFeatures* device_features);

void add_device_feature_extensions(const DeviceFeatures& features, std::vector<std::string>* extensions);
}
