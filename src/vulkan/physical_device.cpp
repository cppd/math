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

#include "physical_device.h"

#include "api_version.h"
#include "error.h"
#include "extensions.h"
#include "features.h"
#include "overview.h"
#include "print.h"
#include "surface.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>

#include <algorithm>
#include <random>
#include <sstream>

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

int device_priority(const PhysicalDevice& physical_device)
{
        const VkPhysicalDeviceType type = physical_device.properties().properties_10.deviceType;

        if (type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
                return 3;
        }

        if (type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
                return 2;
        }

        if (type == VK_PHYSICAL_DEVICE_TYPE_CPU)
        {
                return 1;
        }

        return 0;
}

PhysicalDevice find_best_physical_device(std::vector<PhysicalDevice>&& physical_devices)
{
        ASSERT(!physical_devices.empty());

        std::size_t best_i = 0;
        auto best_priority = device_priority(physical_devices[0]);

        for (std::size_t i = 1; i < physical_devices.size(); ++i)
        {
                const auto priority = device_priority(physical_devices[i]);
                if (priority > best_priority)
                {
                        best_priority = priority;
                        best_i = i;
                }
        }

        return std::move(physical_devices[best_i]);
}

PhysicalDevice find_random_physical_device(std::vector<PhysicalDevice>&& physical_devices)
{
        ASSERT(!physical_devices.empty());

        PCG engine;
        std::uniform_int_distribution<std::size_t> uid(0, physical_devices.size() - 1);

        return std::move(physical_devices[uid(engine)]);
}

PhysicalDevice find_physical_device(
        const PhysicalDeviceSearchType search_type,
        std::vector<PhysicalDevice>&& physical_devices)
{
        if (physical_devices.empty())
        {
                error("Failed to find a suitable Vulkan physical device");
        }

        switch (search_type)
        {
        case PhysicalDeviceSearchType::BEST:
                return find_best_physical_device(std::move(physical_devices));
        case PhysicalDeviceSearchType::RANDOM:
                return find_random_physical_device(std::move(physical_devices));
        }

        error("Unknown physical device search type " + to_string(enum_to_int(search_type)));
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

const PhysicalDeviceInfo& PhysicalDevice::info() const
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

const PhysicalDeviceProperties& PhysicalDevice::properties() const
{
        return info_.properties;
}

const PhysicalDeviceFeatures& PhysicalDevice::features() const
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

bool PhysicalDevice::queue_family_supports_presentation(const std::uint32_t index) const
{
        ASSERT(index < presentation_support_.size());

        return presentation_support_[index];
}

//

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

PhysicalDevice find_physical_device(
        const PhysicalDeviceSearchType search_type,
        const VkInstance instance,
        const VkSurfaceKHR surface,
        const DeviceFunctionality& device_functionality)
{
        LOG(overview_physical_devices(instance, surface));

        std::vector<PhysicalDevice> physical_devices;

        for (const VkPhysicalDevice device : find_physical_devices(instance))
        {
                PhysicalDevice physical_device(device, surface);

                if (!check_features(device_functionality.required_features, physical_device.features()))
                {
                        continue;
                }

                if (!std::all_of(
                            device_functionality.required_extensions.cbegin(),
                            device_functionality.required_extensions.cend(),
                            [&](const std::string& e)
                            {
                                    return physical_device.extensions().contains(e);
                            }))
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

        return find_physical_device(search_type, std::move(physical_devices));
}
}
