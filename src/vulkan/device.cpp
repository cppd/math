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

#include "device.h"

#include "device_info.h"
#include "features.h"
#include "overview.h"
#include "print.h"
#include "surface.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/vector.h>

#include <algorithm>

namespace ns::vulkan
{
namespace
{
bool find_family(
        const std::vector<VkQueueFamilyProperties>& families,
        const VkQueueFlags flags,
        const VkQueueFlags no_flags,
        std::uint32_t* const index)
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

int device_priority(const PhysicalDevice& physical_device)
{
        const VkPhysicalDeviceType type = physical_device.properties().properties_10.deviceType;

        if (type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
                return 0;
        }

        if (type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
                return 1;
        }

        if (type == VK_PHYSICAL_DEVICE_TYPE_CPU)
        {
                return 2;
        }

        return 3;
}

PhysicalDevice find_best_physical_device(std::vector<PhysicalDevice>&& physical_devices)
{
        if (physical_devices.empty())
        {
                error("Failed to find a suitable Vulkan physical device");
        }

        std::size_t best_i = 0;
        auto best_priority = device_priority(physical_devices[0]);

        for (std::size_t i = 1; i < physical_devices.size(); ++i)
        {
                const auto priority = device_priority(physical_devices[i]);
                if (priority < best_priority)
                {
                        best_priority = priority;
                        best_i = i;
                }
        }

        return std::move(physical_devices[best_i]);
}

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

PhysicalDevice::PhysicalDevice(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface)
        : physical_device_(physical_device),
          info_(find_physical_device_info(physical_device)),
          presentation_support_(find_queue_family_presentation_support(surface, physical_device_))
{
        ASSERT(physical_device_ != VK_NULL_HANDLE);
        ASSERT(info_.queue_families.size() == presentation_support_.size());
}

const DeviceInfo& PhysicalDevice::info() const
{
        return info_;
}

VkPhysicalDevice PhysicalDevice::device() const
{
        return physical_device_;
}

const std::unordered_set<std::string>& PhysicalDevice::extensions() const
{
        return info_.extensions;
}

const DeviceProperties& PhysicalDevice::properties() const
{
        return info_.properties;
}

const DeviceFeatures& PhysicalDevice::features() const
{
        return info_.features;
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::queue_families() const
{
        return info_.queue_families;
}

std::uint32_t PhysicalDevice::family_index(
        const VkQueueFlags set_flags,
        const VkQueueFlags not_set_flags,
        const std::vector<VkQueueFlags>& default_flags) const
{
        std::uint32_t index;

        if (set_flags && find_family(info_.queue_families, set_flags, not_set_flags, &index))
        {
                return index;
        }

        for (const VkQueueFlags flags : default_flags)
        {
                if (flags && find_family(info_.queue_families, flags, 0, &index))
                {
                        return index;
                }
        }

        error("Queue family not found, flags set " + to_string(set_flags) + "; not set " + to_string(not_set_flags)
              + "; default " + to_string(default_flags));
}

std::uint32_t PhysicalDevice::presentation_family_index() const
{
        for (std::size_t family_index = 0; family_index < presentation_support_.size(); ++family_index)
        {
                if (presentation_support_[family_index])
                {
                        return family_index;
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
                        return info_.extensions.contains(e);
                });
}

bool PhysicalDevice::queue_family_supports_presentation(const std::uint32_t index) const
{
        ASSERT(index < presentation_support_.size());

        return presentation_support_[index];
}

//

Device::Device(const PhysicalDevice* const physical_device, const VkDeviceCreateInfo& create_info)
        : device_(physical_device->device(), create_info),
          physical_device_(physical_device),
          features_(extract_device_features(create_info))
{
        ASSERT(!create_info.pEnabledFeatures);

        for (unsigned i = 0; i < create_info.queueCreateInfoCount; ++i)
        {
                const std::uint32_t family_index = create_info.pQueueCreateInfos[i].queueFamilyIndex;
                const std::uint32_t queue_count = create_info.pQueueCreateInfos[i].queueCount;
                const auto [iter, inserted] = queues_.try_emplace(family_index);

                if (!inserted)
                {
                        error("Non unique device queue family indices");
                }

                for (std::uint32_t queue_index = 0; queue_index < queue_count; ++queue_index)
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

Queue Device::queue(const std::uint32_t family_index, const std::uint32_t queue_index) const
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

PhysicalDevice find_physical_device(
        const VkInstance instance,
        const VkSurfaceKHR surface,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& required_features)
{
        sort_and_unique(&required_extensions);

        LOG(overview_physical_devices(instance, surface));

        std::vector<PhysicalDevice> physical_devices;

        for (const VkPhysicalDevice device : find_physical_devices(instance))
        {
                PhysicalDevice physical_device(device, surface);

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

                physical_devices.push_back(std::move(physical_device));
        }

        return find_best_physical_device(std::move(physical_devices));
}

Device create_device(
        const PhysicalDevice* const physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& required_features,
        const DeviceFeatures& optional_features)
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

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();

        DeviceFeatures features = make_features(required_features, optional_features, physical_device->features());

        add_device_feature_extensions(features, &required_extensions);
        sort_and_unique(&required_extensions);
        for (const std::string& extension : required_extensions)
        {
                if (!physical_device->extensions().contains(extension))
                {
                        error("Vulkan physical device does not support extension " + extension);
                }
        }

        std::string info;

        info = std::string("Vulkan device name: ")
               + static_cast<const char*>(physical_device->properties().properties_10.deviceName);
        info += "\nVulkan device API version: "
                + api_version_to_string(physical_device->properties().properties_10.apiVersion);

        const std::vector<const char*> extensions = const_char_pointer_vector(required_extensions);
        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();
        info += "\nVulkan device extensions: {" + strings_to_sorted_string(extensions) + "}";

        info += "\nVulkan device features: {" + strings_to_sorted_string(features_to_strings(features, true)) + "}";

        VkPhysicalDeviceFeatures2 features_2;
        make_device_features(&features_2, &features);
        create_info.pNext = &features_2;

        LOG(info);

        return Device(physical_device, create_info);
}
}
