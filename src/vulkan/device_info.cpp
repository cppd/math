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

#include "device_info.h"

#include "error.h"
#include "print.h"
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
        oss << api_version_to_string(properties.apiVersion);
        oss << " is not supported, minimum required version is ";
        oss << api_version_to_string(API_VERSION);
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

        std::unordered_set<std::string> extension_set;
        for (const VkExtensionProperties& properties : extensions)
        {
                extension_set.emplace(properties.extensionName);
        }
        return extension_set;
}

DeviceProperties find_properties(const VkPhysicalDevice device)
{
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

DeviceFeatures find_features(const VkPhysicalDevice device)
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
}

std::vector<VkPhysicalDevice> find_physical_devices(const VkInstance instance)
{
        std::uint32_t count;
        VULKAN_CHECK(vkEnumeratePhysicalDevices(instance, &count, nullptr));

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

                if (api_version_suitable(properties.apiVersion))
                {
                        devices.push_back(device);
                }
        }

        if (!devices.empty())
        {
                return devices;
        }

        std::ostringstream oss;
        oss << "No Vulkan physical device found with minimum supported version ";
        oss << api_version_to_string(API_VERSION);
        oss << '\n';
        oss << "Found " << (all_devices.size() > 1 ? "devices" : "device");
        for (const VkPhysicalDevice device : all_devices)
        {
                oss << '\n';
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);
                oss << static_cast<const char*>(properties.deviceName) << "\n";
                oss << "  API version " << api_version_to_string(properties.apiVersion);
        }

        error(oss.str());
}

DeviceInfo find_physical_device_info(const VkPhysicalDevice device)
{
        check_api_version(device);

        DeviceInfo info;
        info.extensions = find_extensions(device);
        info.properties = find_properties(device);
        info.features = find_features(device);
        info.queue_families = find_queue_families(device);
        return info;
}

std::vector<bool> find_queue_family_presentation_support(const VkSurfaceKHR surface, const VkPhysicalDevice device)
{
        std::uint32_t family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);

        std::vector<bool> presentation_support(family_count, false);

        if (surface == VK_NULL_HANDLE)
        {
                return presentation_support;
        }

        for (std::uint32_t family_index = 0; family_index < family_count; ++family_index)
        {
                VkBool32 supported;
                VULKAN_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, family_index, surface, &supported));

                presentation_support[family_index] = (supported == VK_TRUE);
        }

        return presentation_support;
}

void add_device_features(VkPhysicalDeviceFeatures2* const features_2, DeviceFeatures* const features)
{
        *features_2 = {};
        features_2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        features_2->features = features->features_10;
        features_2->pNext = &features->features_11;

        features->features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        features->features_11.pNext = &features->features_12;

        features->features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features->features_12.pNext = nullptr;
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
