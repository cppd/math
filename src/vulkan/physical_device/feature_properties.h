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

#include <cstddef>
#include <string>
#include <vulkan/vulkan_core.h>

namespace ns::vulkan
{
template <typename T>
struct FeatureProperties;

template <>
struct FeatureProperties<VkPhysicalDeviceFeatures> final
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceFeatures, robustBufferAccess);
        static constexpr std::size_t COUNT = 55;
        static std::string name(std::size_t index);
};

template <>
struct FeatureProperties<VkPhysicalDeviceVulkan11Features> final
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceVulkan11Features, storageBuffer16BitAccess);
        static constexpr std::size_t COUNT = 12;
        static std::string name(std::size_t index);
};

template <>
struct FeatureProperties<VkPhysicalDeviceVulkan12Features> final
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceVulkan12Features, samplerMirrorClampToEdge);
        static constexpr std::size_t COUNT = 47;
        static std::string name(std::size_t index);
};

template <>
struct FeatureProperties<VkPhysicalDeviceVulkan13Features> final
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceVulkan13Features, robustImageAccess);
        static constexpr std::size_t COUNT = 15;
        static std::string name(std::size_t index);
};

template <>
struct FeatureProperties<VkPhysicalDeviceAccelerationStructureFeaturesKHR> final
{
        static constexpr std::size_t OFFSET =
                offsetof(VkPhysicalDeviceAccelerationStructureFeaturesKHR, accelerationStructure);
        static constexpr std::size_t COUNT = 5;
        static std::string name(std::size_t index);
};

template <>
struct FeatureProperties<VkPhysicalDeviceRayQueryFeaturesKHR> final
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceRayQueryFeaturesKHR, rayQuery);
        static constexpr std::size_t COUNT = 1;
        static std::string name(std::size_t index);
};

template <>
struct FeatureProperties<VkPhysicalDeviceRayTracingPipelineFeaturesKHR> final
{
        static constexpr std::size_t OFFSET =
                offsetof(VkPhysicalDeviceRayTracingPipelineFeaturesKHR, rayTracingPipeline);
        static constexpr std::size_t COUNT = 5;
        static std::string name(std::size_t index);
};
}
