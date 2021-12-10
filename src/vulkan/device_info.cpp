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

#include "device_info.h"

#include "error.h"
#include "settings.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <cstring>
#include <sstream>

namespace ns::vulkan
{
namespace
{
std::uint32_t find_device_extension_count(const VkPhysicalDevice device)
{
        std::uint32_t count;
        VULKAN_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr));
        return count;
}

std::uint32_t find_physical_device_count(const VkInstance instance)
{
        std::uint32_t count;
        VULKAN_CHECK(vkEnumeratePhysicalDevices(instance, &count, nullptr));
        return count;
}
}

std::vector<bool> find_presentation_support(
        const VkSurfaceKHR surface,
        const VkPhysicalDevice device,
        const std::vector<VkQueueFamilyProperties>& queue_families)
{
        if (surface == VK_NULL_HANDLE)
        {
                return std::vector<bool>(queue_families.size(), false);
        }

        std::vector<bool> presentation_supported(queue_families.size());

        for (std::uint32_t i = 0; i < queue_families.size(); ++i)
        {
                if (queue_families[i].queueCount < 1)
                {
                        continue;
                }

                VkBool32 supported;

                VULKAN_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported));

                presentation_supported[i] = (supported == VK_TRUE);
        }

        return presentation_supported;
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

std::unordered_set<std::string> find_device_extensions(const VkPhysicalDevice device)
{
        std::uint32_t count = find_device_extension_count(device);

        if (count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(count);

        VULKAN_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()));

        std::unordered_set<std::string> extension_set;
        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }
        return extension_set;
}

std::vector<VkPhysicalDevice> find_physical_devices(const VkInstance instance)
{
        std::uint32_t count = find_physical_device_count(instance);
        if (count < 1)
        {
                error("No Vulkan physical device found");
        }

        std::vector<VkPhysicalDevice> all_devices(count);
        VULKAN_CHECK(vkEnumeratePhysicalDevices(instance, &count, all_devices.data()));

        std::vector<VkPhysicalDevice> devices;
        for (const VkPhysicalDevice device : all_devices)
        {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.apiVersion < API_VERSION)
                {
                        continue;
                }
                devices.push_back(device);
        }

        if (!devices.empty())
        {
                return devices;
        }

        std::ostringstream oss;
        oss << "No Vulkan physical devices found with minimum supported version ";
        oss << API_VERSION_MAJOR << "." << API_VERSION_MINOR;
        oss << '\n';
        oss << "Found " << (all_devices.size() > 1 ? "devices" : "device");
        for (const VkPhysicalDevice& d : all_devices)
        {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(d, &properties);
                oss << "\n";
                oss << static_cast<const char*>(properties.deviceName) << "\n  API version "
                    << VK_VERSION_MAJOR(properties.apiVersion) << "." << VK_VERSION_MINOR(properties.apiVersion);
        }
        error(oss.str());
}

DeviceProperties find_physical_device_properties(const VkPhysicalDevice device)
{
        {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.apiVersion < API_VERSION)
                {
                        std::ostringstream oss;
                        oss << "Vulkan physical device version ";
                        oss << VK_VERSION_MAJOR(properties.apiVersion) << "."
                            << VK_VERSION_MINOR(properties.apiVersion);
                        oss << " is not supported, minimum required version is ";
                        oss << API_VERSION_MAJOR << "." << API_VERSION_MINOR;
                        error(oss.str());
                }
        }

        VkPhysicalDeviceVulkan12Properties vulkan_12_properties = {};
        vulkan_12_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
        VkPhysicalDeviceVulkan11Properties vulkan_11_properties = {};
        vulkan_11_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
        vulkan_11_properties.pNext = &vulkan_12_properties;
        VkPhysicalDeviceProperties2 properties_2 = {};
        properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties_2.pNext = &vulkan_11_properties;
        vkGetPhysicalDeviceProperties2(device, &properties_2);

        DeviceProperties res;
        res.properties_10 = properties_2.properties;
        res.properties_11 = vulkan_11_properties;
        res.properties_11.pNext = nullptr;
        res.properties_12 = vulkan_12_properties;
        res.properties_12.pNext = nullptr;
        return res;
}

DeviceFeatures find_physical_device_features(const VkPhysicalDevice device)
{
        VkPhysicalDeviceVulkan12Features vulkan_12_features = {};
        vulkan_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        VkPhysicalDeviceVulkan11Features vulkan_11_features = {};
        vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        vulkan_11_features.pNext = &vulkan_12_features;
        VkPhysicalDeviceFeatures2 features_2 = {};
        features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features_2.pNext = &vulkan_11_features;
        vkGetPhysicalDeviceFeatures2(device, &features_2);

        DeviceFeatures res;
        res.features_10 = features_2.features;
        res.features_11 = vulkan_11_features;
        res.features_11.pNext = nullptr;
        res.features_12 = vulkan_12_features;
        res.features_12.pNext = nullptr;
        return res;
}

DeviceFeatures extract_device_features(const VkDeviceCreateInfo& create_info)
{
        DeviceFeatures features;

        bool features_10 = false;
        bool features_11 = false;
        bool features_12 = false;

        const void* ptr = create_info.pNext;

        while (ptr)
        {
                VkStructureType type;
                std::memcpy(&type, ptr, sizeof(VkStructureType));
                if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2)
                {
                        if (features_10)
                        {
                                error("Unique device features required");
                        }
                        features_10 = true;
                        VkPhysicalDeviceFeatures2 features_2;
                        std::memcpy(&features_2, ptr, sizeof(VkPhysicalDeviceFeatures2));
                        ptr = features_2.pNext;
                        features.features_10 = features_2.features;
                }
                else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES)
                {
                        if (features_11)
                        {
                                error("Unique device features required");
                        }
                        features_11 = true;
                        std::memcpy(&features.features_11, ptr, sizeof(VkPhysicalDeviceVulkan11Features));
                        ptr = features.features_11.pNext;
                        features.features_11.pNext = nullptr;
                }
                else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES)
                {
                        if (features_12)
                        {
                                error("Unique device features required");
                        }
                        features_12 = true;
                        std::memcpy(&features.features_12, ptr, sizeof(VkPhysicalDeviceVulkan12Features));
                        ptr = features.features_12.pNext;
                        features.features_12.pNext = nullptr;
                }
                else
                {
                        error("Unknown device create info type " + to_string(enum_to_int(type)));
                }
        }

        if (!features_10 || !features_11 || !features_12)
        {
                error("Not all device features specified for device creation");
        }

        return features;
}
}
