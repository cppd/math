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

#include "create.h"

#include "../physical_device/features.h"
#include "../print.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/string/vector.h>

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
}

handle::Device create_device(
        const PhysicalDevice* const physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
        const std::unordered_set<std::string>& required_extensions,
        const PhysicalDeviceFeatures& required_features)
{
        check_queue_families(*physical_device, queue_families);

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

        std::string info;

        info = std::string("Vulkan device name: ")
               + static_cast<const char*>(physical_device->properties().properties_10.deviceName);
        info += "\nVulkan device API version: "
                + api_version_to_string(physical_device->properties().properties_10.apiVersion);

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();

        for (const std::string& extension : required_extensions)
        {
                if (!physical_device->extensions().contains(extension))
                {
                        error("Vulkan physical device does not support required extension " + extension);
                }
        }
        const std::vector<const char*> extensions = const_char_pointer_vector(&required_extensions);
        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();
        info += "\nVulkan device extensions: {" + strings_to_sorted_string(extensions) + "}";

        VkPhysicalDeviceFeatures2 features_2;
        PhysicalDeviceFeatures features;
        make_physical_device_features(required_features, &features_2, &features);
        create_info.pNext = &features_2;
        info += "\nVulkan device features: {" + strings_to_sorted_string(features_to_strings(required_features, true))
                + "}";

        LOG(info);

        return {physical_device->device(), create_info};
}
}
