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

#include "create.h"

#include "../physical_device/features.h"
#include "../strings.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/string/strings.h>

#include <algorithm>

namespace ns::vulkan
{
namespace
{
void check_queue_families(
        const PhysicalDevice& physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families)
{
        if (queue_families.empty())
        {
                error("No queue families for device creation");
        }

        if (!std::all_of(
                    queue_families.cbegin(), queue_families.cend(),
                    [&](const auto& v)
                    {
                            return v.first < physical_device.queue_families().size();
                    }))
        {
                error("Error queue families");
        }

        if (!std::all_of(
                    queue_families.cbegin(), queue_families.cend(),
                    [](const auto& v)
                    {
                            return v.second > 0;
                    }))
        {
                error("Error queue families");
        }

        if (!std::all_of(
                    queue_families.cbegin(), queue_families.cend(),
                    [&](const auto& v)
                    {
                            return v.second <= physical_device.queue_families()[v.first].queueCount;
                    }))
        {
                error("Error queue families");
        }
}

void check_required_extensions(
        const PhysicalDevice& physical_device,
        const std::unordered_set<std::string>& required_extensions)
{
        for (const std::string& extension : required_extensions)
        {
                if (!physical_device.extensions().contains(extension))
                {
                        error("Vulkan physical device does not support required extension " + extension);
                }
        }
}

std::string info_string(
        const PhysicalDevice& physical_device,
        const std::unordered_set<std::string>& required_extensions,
        const PhysicalDeviceFeatures& required_features)
{
        std::string info;

        info += "Vulkan device name: ";
        info += static_cast<const char*>(physical_device.properties().properties_10.deviceName);

        info += "\nVulkan device API version: ";
        info += api_version_to_string(physical_device.properties().properties_10.apiVersion);

        info += "\nVulkan device extensions: {";
        info += strings_to_sorted_string(required_extensions, ", ");
        info += "}";

        info += "\nVulkan device features: {";
        info += strings_to_sorted_string(features_to_strings(required_features, true), ", ");
        info += "}";

        return info;
}
}

handle::Device create_device(
        const PhysicalDevice* const physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
        const std::unordered_set<std::string>& required_extensions,
        const PhysicalDeviceFeatures& required_features)
{
        check_queue_families(*physical_device, queue_families);
        check_required_extensions(*physical_device, required_extensions);

        LOG(info_string(*physical_device, required_extensions, required_features));

        std::vector<std::vector<float>> queue_priorities(queue_families.size());
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_families.size());

        for (unsigned i = 0; const auto& [queue_family_index, queue_count] : queue_families)
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

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();

        const std::vector<const char*> extensions = strings_to_char_pointers(required_extensions);
        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();

        VkPhysicalDeviceFeatures2 features_2;
        PhysicalDeviceFeatures features;
        make_physical_device_features(required_features, &features_2, &features);
        create_info.pNext = &features_2;

        return {physical_device->device(), create_info};
}
}
