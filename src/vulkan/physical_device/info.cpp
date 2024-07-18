/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "info.h"

#include "features.h"

#include <src/com/error.h>
#include <src/vulkan/api_version.h>
#include <src/vulkan/error.h>
#include <src/vulkan/strings.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace ns::vulkan::physical_device
{
namespace
{
void check_api_version(const VkPhysicalDevice device)
{
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        if (api_version_suitable(properties.apiVersion))
        {
                return;
        }

        std::ostringstream oss;
        oss << "Vulkan physical device version ";
        oss << strings::api_version_to_string(properties.apiVersion);
        oss << " is not supported, minimum required version is ";
        oss << strings::api_version_to_string(API_VERSION);
        error(oss.str());
}

std::unordered_set<std::string> find_extensions(const VkPhysicalDevice device)
{
        std::uint32_t count;
        VULKAN_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr));

        if (count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(count);
        VULKAN_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()));

        std::unordered_set<std::string> res;
        res.reserve(extensions.size());
        for (const VkExtensionProperties& extension : extensions)
        {
                res.emplace(extension.extensionName);
        }
        return res;
}

template <typename T>
void connect(void**& last, T& s)
{
        if (last)
        {
                *last = &s;
        }
        s.pNext = nullptr;
        last = &s.pNext;
}

void set_nullptr_next(Properties* const properties)
{
        properties->properties_11.pNext = nullptr;
        properties->properties_12.pNext = nullptr;
        properties->properties_13.pNext = nullptr;
        if (properties->acceleration_structure)
        {
                properties->acceleration_structure->pNext = nullptr;
        }
        if (properties->ray_tracing_pipeline)
        {
                properties->ray_tracing_pipeline->pNext = nullptr;
        }
}

void set_nullptr_next(Features* const features)
{
        features->features_11.pNext = nullptr;
        features->features_12.pNext = nullptr;
        features->features_13.pNext = nullptr;
        features->acceleration_structure.pNext = nullptr;
        features->ray_query.pNext = nullptr;
        features->ray_tracing_pipeline.pNext = nullptr;
}

Properties find_properties(const VkPhysicalDevice device, const std::unordered_set<std::string>& extensions)
{
        Properties res;

        void** last = nullptr;

        VkPhysicalDeviceProperties2 properties_2;
        properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        connect(last, properties_2);

        res.properties_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
        connect(last, res.properties_11);

        res.properties_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
        connect(last, res.properties_12);

        res.properties_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
        connect(last, res.properties_13);

        if (extensions.contains(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
        {
                res.acceleration_structure.emplace();
                res.acceleration_structure->sType =
                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
                connect(last, *res.acceleration_structure);
        }

        if (extensions.contains(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
        {
                res.ray_tracing_pipeline.emplace();
                res.ray_tracing_pipeline->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
                connect(last, *res.ray_tracing_pipeline);
        }

        vkGetPhysicalDeviceProperties2(device, &properties_2);

        res.properties_10 = properties_2.properties;

        set_nullptr_next(&res);

        return res;
}

Features find_features(const VkPhysicalDevice device, const std::unordered_set<std::string>& extensions)
{
        Features res;

        void** last = nullptr;

        VkPhysicalDeviceFeatures2 features_2;
        features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        connect(last, features_2);

        res.features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        connect(last, res.features_11);

        res.features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        connect(last, res.features_12);

        res.features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        connect(last, res.features_13);

        if (extensions.contains(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
        {
                res.acceleration_structure.sType =
                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
                connect(last, res.acceleration_structure);
        }
        else
        {
                res.acceleration_structure = {};
        }

        if (extensions.contains(VK_KHR_RAY_QUERY_EXTENSION_NAME))
        {
                res.ray_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
                connect(last, res.ray_query);
        }
        else
        {
                res.ray_query = {};
        }

        if (extensions.contains(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
        {
                res.ray_tracing_pipeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
                connect(last, res.ray_tracing_pipeline);
        }
        else
        {
                res.ray_tracing_pipeline = {};
        }

        vkGetPhysicalDeviceFeatures2(device, &features_2);

        res.features_10 = features_2.features;

        set_nullptr_next(&res);

        return res;
}

std::vector<VkQueueFamilyProperties> find_queue_families(const VkPhysicalDevice device)
{
        std::uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

        if (count < 1)
        {
                return {};
        }

        std::vector<VkQueueFamilyProperties> queue_families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queue_families.data());

        return queue_families;
}

std::vector<std::string> extensions_for_features(const Features& features)
{
        std::vector<std::string> extensions;

        const auto add_acceleration_structure_extensions = [&]
        {
                extensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                extensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        };

        if (any_feature_enabled(features.acceleration_structure))
        {
                add_acceleration_structure_extensions();
        }

        if (any_feature_enabled(features.ray_query))
        {
                add_acceleration_structure_extensions();
                extensions.emplace_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        }

        if (any_feature_enabled(features.ray_tracing_pipeline))
        {
                add_acceleration_structure_extensions();
                extensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        }

        return extensions;
}
}

DeviceInfo device_info(const VkPhysicalDevice device)
{
        check_api_version(device);

        DeviceInfo info;
        info.extensions = find_extensions(device);
        info.properties = find_properties(device, info.extensions);
        info.features = find_features(device, info.extensions);
        info.queue_families = find_queue_families(device);
        return info;
}

void make_features(
        const Features& features,
        VkPhysicalDeviceFeatures2* const features_2,
        Features* const device_features)
{
        *device_features = features;

        void** last = nullptr;

        features_2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features_2->features = device_features->features_10;
        connect(last, *features_2);

        device_features->features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        connect(last, device_features->features_11);

        device_features->features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        connect(last, device_features->features_12);

        device_features->features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        connect(last, device_features->features_13);

        if (any_feature_enabled(device_features->acceleration_structure))
        {
                device_features->acceleration_structure.sType =
                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
                connect(last, device_features->acceleration_structure);
        }

        if (any_feature_enabled(device_features->ray_query))
        {
                device_features->ray_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
                connect(last, device_features->ray_query);
        }

        if (any_feature_enabled(device_features->ray_tracing_pipeline))
        {
                device_features->ray_tracing_pipeline.sType =
                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
                connect(last, device_features->ray_tracing_pipeline);
        }
}

std::unordered_set<std::string> make_extensions(
        const Features& required_features,
        const std::unordered_set<std::string>& required_extensions,
        const std::unordered_set<std::string>& optional_extensions,
        const std::unordered_set<std::string>& supported_extensions)
{
        std::unordered_set<std::string> res;

        for (const std::string& extension : required_extensions)
        {
                if (!supported_extensions.contains(extension))
                {
                        error("Vulkan physical device does not support required extension " + extension);
                }
                res.insert(extension);
        }

        for (const std::string& extension : extensions_for_features(required_features))
        {
                if (!supported_extensions.contains(extension))
                {
                        error("Vulkan physical device does not support required feature extension " + extension);
                }
                res.insert(extension);
        }

        for (const std::string& extension : optional_extensions)
        {
                if (supported_extensions.contains(extension))
                {
                        res.insert(extension);
                }
        }

        return res;
}
}
