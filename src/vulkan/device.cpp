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

#include "device.h"

#include "error.h"
#include "features.h"
#include "overview.h"
#include "query.h"
#include "settings.h"
#include "surface.h"

#include <src/com/alg.h>
#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/vector.h>

#include <algorithm>
#include <sstream>

namespace ns::vulkan
{
namespace
{
bool find_family(
        const std::vector<VkQueueFamilyProperties>& families,
        VkQueueFlags flags,
        VkQueueFlags no_flags,
        uint32_t* index)
{
        ASSERT(flags != 0);
        ASSERT((flags & no_flags) == 0);

        for (std::size_t i = 0; i < families.size(); ++i)
        {
                const VkQueueFamilyProperties& p = families[i];

                if (p.queueCount < 1)
                {
                        continue;
                }

                if (((p.queueFlags & flags) == flags) && !(p.queueFlags & no_flags))
                {
                        *index = i;
                        return true;
                }
        }
        return false;
}

std::vector<bool> find_presentation_support(
        VkSurfaceKHR surface,
        VkPhysicalDevice device,
        const std::vector<VkQueueFamilyProperties>& queue_families)
{
        if (surface == VK_NULL_HANDLE)
        {
                return std::vector<bool>(queue_families.size(), false);
        }

        std::vector<bool> presentation_supported(queue_families.size());

        for (uint32_t i = 0; i < queue_families.size(); ++i)
        {
                if (queue_families[i].queueCount < 1)
                {
                        continue;
                }

                VkBool32 supported;

                const VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
                if (result != VK_SUCCESS)
                {
                        vulkan_function_error("vkGetPhysicalDeviceSurfaceSupportKHR", result);
                }

                presentation_supported[i] = (supported == VK_TRUE);
        }

        return presentation_supported;
}

std::vector<VkQueueFamilyProperties> find_queue_families(VkPhysicalDevice device)
{
        uint32_t queue_family_count;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        if (queue_family_count < 1)
        {
                return {};
        }

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        return queue_families;
}

uint32_t find_extension_count(VkPhysicalDevice device)
{
        uint32_t extension_count;
        const VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }
        return extension_count;
}

std::unordered_set<std::string> find_extensions(VkPhysicalDevice device)
{
        uint32_t extension_count = find_extension_count(device);
        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);

        const VkResult result =
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }

        std::unordered_set<std::string> extension_set;
        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }
        return extension_set;
}

DeviceFeatures device_features(const VkDeviceCreateInfo& create_info)
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

uint32_t find_physical_device_count(VkInstance instance)
{
        uint32_t device_count;
        const VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumeratePhysicalDevices", result);
        }
        return device_count;
}
}

//

PhysicalDevice::PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
        : physical_device_(physical_device)
{
        ASSERT(physical_device != VK_NULL_HANDLE);

        {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(physical_device, &properties);
                if (properties.apiVersion < API_VERSION)
                {
                        std::ostringstream oss;
                        oss << "Vulkan physical device version ";
                        oss << VK_VERSION_MAJOR(properties.apiVersion) << "."
                            << VK_VERSION_MINOR(properties.apiVersion);
                        oss << " is not supported, minimum version is ";
                        oss << API_VERSION_MAJOR << "." << API_VERSION_MINOR;
                        error(oss.str());
                }
        }
        {
                VkPhysicalDeviceVulkan12Properties vulkan_12_properties = {};
                vulkan_12_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
                VkPhysicalDeviceVulkan11Properties vulkan_11_properties = {};
                vulkan_11_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
                vulkan_11_properties.pNext = &vulkan_12_properties;
                VkPhysicalDeviceProperties2 properties_2 = {};
                properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                properties_2.pNext = &vulkan_11_properties;
                vkGetPhysicalDeviceProperties2(physical_device_, &properties_2);

                properties_.properties_10 = properties_2.properties;
                properties_.properties_11 = vulkan_11_properties;
                properties_.properties_11.pNext = nullptr;
                properties_.properties_12 = vulkan_12_properties;
                properties_.properties_12.pNext = nullptr;
        }
        {
                VkPhysicalDeviceVulkan12Features vulkan_12_features = {};
                vulkan_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
                VkPhysicalDeviceVulkan11Features vulkan_11_features = {};
                vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
                vulkan_11_features.pNext = &vulkan_12_features;
                VkPhysicalDeviceFeatures2 features_2 = {};
                features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features_2.pNext = &vulkan_11_features;
                vkGetPhysicalDeviceFeatures2(physical_device_, &features_2);

                features_.features_10 = features_2.features;
                features_.features_11 = vulkan_11_features;
                features_.features_11.pNext = nullptr;
                features_.features_12 = vulkan_12_features;
                features_.features_12.pNext = nullptr;
        }

        queue_families_ = find_queue_families(physical_device);
        presentation_supported_ = find_presentation_support(surface, physical_device_, queue_families_);
        supported_extensions_ = find_extensions(physical_device_);

        ASSERT(queue_families_.size() == presentation_supported_.size());
}

VkPhysicalDevice PhysicalDevice::device() const
{
        return physical_device_;
}

const DeviceFeatures& PhysicalDevice::features() const
{
        return features_;
}

const DeviceProperties& PhysicalDevice::properties() const
{
        return properties_;
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::queue_families() const
{
        return queue_families_;
}

const std::unordered_set<std::string>& PhysicalDevice::supported_extensions() const
{
        return supported_extensions_;
}

uint32_t PhysicalDevice::family_index(
        VkQueueFlags set_flags,
        VkQueueFlags not_set_flags,
        const std::vector<VkQueueFlags>& default_flags) const
{
        uint32_t index;
        if (set_flags && find_family(queue_families_, set_flags, not_set_flags, &index))
        {
                return index;
        }
        for (const VkQueueFlags& flags : default_flags)
        {
                if (flags && find_family(queue_families_, flags, 0, &index))
                {
                        return index;
                }
        }
        error("Queue family not found, flags set " + to_string(set_flags) + "; not set " + to_string(not_set_flags)
              + "; default " + to_string(default_flags));
}

uint32_t PhysicalDevice::presentation_family_index() const
{
        for (std::size_t i = 0; i < presentation_supported_.size(); ++i)
        {
                if (presentation_supported_[i])
                {
                        return i;
                }
        }
        error("Presentation family not found");
}

bool PhysicalDevice::supports_extensions(const std::vector<std::string>& extensions) const
{
        return std::all_of(
                extensions.cbegin(), extensions.cend(),
                [&](const std::string& e)
                {
                        return supported_extensions_.count(e) >= 1;
                });
}

bool PhysicalDevice::queue_family_supports_presentation(uint32_t index) const
{
        ASSERT(index < presentation_supported_.size());

        return presentation_supported_[index];
}

//

Device::Device(const PhysicalDevice* physical_device, const VkDeviceCreateInfo& create_info)
        : device_(physical_device->device(), create_info),
          physical_device_(physical_device),
          features_(device_features(create_info))
{
        ASSERT(!create_info.pEnabledFeatures);

        for (unsigned i = 0; i < create_info.queueCreateInfoCount; ++i)
        {
                uint32_t family_index = create_info.pQueueCreateInfos[i].queueFamilyIndex;
                uint32_t queue_count = create_info.pQueueCreateInfos[i].queueCount;
                auto [iter, inserted] = queues_.try_emplace(family_index);
                if (!inserted)
                {
                        error("Non unique device queue family indices");
                }
                for (uint32_t queue_index = 0; queue_index < queue_count; ++queue_index)
                {
                        VkQueue queue;
                        vkGetDeviceQueue(device_, family_index, queue_index, &queue);
                        if (queue == VK_NULL_HANDLE)
                        {
                                error("Null queue handle");
                        }
                        iter->second.push_back(queue);
                }
        }
}

VkPhysicalDevice Device::physical_device() const
{
        return physical_device_->device();
}

const DeviceFeatures& Device::features() const
{
        return features_;
}

const DeviceProperties& Device::properties() const
{
        return physical_device_->properties();
}

Queue Device::queue(uint32_t family_index, uint32_t queue_index) const
{
        const auto iter = queues_.find(family_index);
        if (iter == queues_.cend())
        {
                error("Queue family index " + to_string(family_index) + " not found");
        }
        if (queue_index >= iter->second.size())
        {
                error("Queue " + to_string(queue_index) + " not found");
        }
        return {family_index, iter->second[queue_index]};
}

//

std::vector<VkPhysicalDevice> physical_devices(VkInstance instance)
{
        uint32_t device_count = find_physical_device_count(instance);
        if (device_count < 1)
        {
                error("No Vulkan device found");
        }

        std::vector<VkPhysicalDevice> all_devices(device_count);
        const VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, all_devices.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumeratePhysicalDevices", result);
        }

        std::vector<VkPhysicalDevice> devices;
        for (const VkPhysicalDevice& d : all_devices)
        {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(d, &properties);
                if (properties.apiVersion < API_VERSION)
                {
                        continue;
                }
                devices.push_back(d);
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

PhysicalDevice create_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& required_features)
{
        sort_and_unique(&required_extensions);

        LOG(overview_physical_devices(instance, surface));

        for (const VkPhysicalDevice& d : physical_devices(instance))
        {
                PhysicalDevice physical_device(d, surface);

                if (physical_device.properties().properties_10.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                    && physical_device.properties().properties_10.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
                    && physical_device.properties().properties_10.deviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU
                    && physical_device.properties().properties_10.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU)
                {
                        continue;
                }

                if (!check_features(required_features, physical_device.features()))
                {
                        continue;
                }

                if (!physical_device.supports_extensions(required_extensions))
                {
                        continue;
                }

                try
                {
                        physical_device.family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, {0});
                }
                catch (...)
                {
                        continue;
                }

                if (surface != VK_NULL_HANDLE)
                {
                        try
                        {
                                physical_device.presentation_family_index();
                        }
                        catch (...)
                        {
                                continue;
                        }

                        if (!surface_suitable(surface, physical_device.device()))
                        {
                                continue;
                        }
                }

                return physical_device;
        }

        error("Failed to find a suitable Vulkan physical device");
}

Device create_device(
        const PhysicalDevice* physical_device,
        const std::unordered_map<uint32_t, uint32_t>& queue_families,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& required_features,
        const DeviceFeatures& optional_features)
{
        sort_and_unique(&required_extensions);

        ASSERT(std::all_of(
                queue_families.cbegin(), queue_families.cend(),
                [&](const auto& v)
                {
                        return v.first < physical_device->queue_families().size();
                }));
        ASSERT(std::all_of(
                queue_families.cbegin(), queue_families.cend(),
                [](const auto& v)
                {
                        return v.second > 0;
                }));
        ASSERT(std::all_of(
                queue_families.cbegin(), queue_families.cend(),
                [&](const auto& v)
                {
                        return v.second <= physical_device->queue_families()[v.first].queueCount;
                }));

        if (queue_families.empty())
        {
                error("No queue families for device creation");
        }

        std::vector<std::vector<float>> queue_priorities(queue_families.size());
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_families.size());
        unsigned i = 0;
        for (const auto& [queue_family_index, queue_count] : queue_families)
        {
                queue_priorities[i].resize(queue_count, 1);

                VkDeviceQueueCreateInfo queue_create_info = {};
                queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.queueFamilyIndex = queue_family_index;
                queue_create_info.queueCount = queue_count;
                queue_create_info.pQueuePriorities = queue_priorities[i].data();
                queue_create_infos[i] = queue_create_info;

                ++i;
        }

        DeviceFeatures features = make_features(required_features, optional_features, physical_device->features());

        features.features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features.features_12.pNext = nullptr;
        features.features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        features.features_11.pNext = &features.features_12;
        VkPhysicalDeviceFeatures2 features_2 = {};
        features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features_2.pNext = &features.features_11;
        features_2.features = features.features_10;

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pNext = &features_2;

        const std::vector<const char*> extensions = const_char_pointer_vector(required_extensions);
        if (!extensions.empty())
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        return Device(physical_device, create_info);
}
}
