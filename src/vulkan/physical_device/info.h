/*
Copyright (C) 2017-2023 Topological Manifold

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
struct PhysicalDeviceProperties final
{
        VkPhysicalDeviceProperties properties_10;
        VkPhysicalDeviceVulkan11Properties properties_11;
        VkPhysicalDeviceVulkan12Properties properties_12;
        VkPhysicalDeviceVulkan13Properties properties_13;
        std::optional<VkPhysicalDeviceAccelerationStructurePropertiesKHR> acceleration_structure;
        std::optional<VkPhysicalDeviceRayTracingPipelinePropertiesKHR> ray_tracing_pipeline;
};

struct PhysicalDeviceFeatures final
{
        VkPhysicalDeviceFeatures features_10{};
        VkPhysicalDeviceVulkan11Features features_11{};
        VkPhysicalDeviceVulkan12Features features_12{};
        VkPhysicalDeviceVulkan13Features features_13{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure{};
        VkPhysicalDeviceRayQueryFeaturesKHR ray_query{};
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline{};
};

struct PhysicalDeviceInfo final
{
        std::unordered_set<std::string> extensions;
        PhysicalDeviceProperties properties;
        PhysicalDeviceFeatures features;
        std::vector<VkQueueFamilyProperties> queue_families;
};

PhysicalDeviceInfo find_physical_device_info(VkPhysicalDevice device);

void make_physical_device_features(
        const PhysicalDeviceFeatures& features,
        VkPhysicalDeviceFeatures2* features_2,
        PhysicalDeviceFeatures* device_features);

std::unordered_set<std::string> make_extensions(
        const PhysicalDeviceFeatures& required_features,
        const std::unordered_set<std::string>& required_extensions,
        const std::unordered_set<std::string>& optional_extensions,
        const std::unordered_set<std::string>& supported_extensions);
}
