/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "find.h"

#include "features.h"
#include "functionality.h"
#include "info.h"
#include "physical_device.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/vulkan/api_version.h>
#include <src/vulkan/error.h>
#include <src/vulkan/overview.h>
#include <src/vulkan/strings.h>
#include <src/vulkan/surface.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ns::vulkan::physical_device
{
namespace
{
constexpr std::uint32_t MIN_STORAGE_BUFFER_RANGE = 1'000'000'000;

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

std::size_t find_best_physical_device(
        const std::vector<PhysicalDevice>& physical_devices,
        const std::vector<std::size_t>& suitable_devices)
{
        ASSERT(!physical_devices.empty() && !suitable_devices.empty());

        std::size_t best_i = 0;
        auto best_priority = device_priority(physical_devices[suitable_devices[0]]);

        for (std::size_t i = 1; i < suitable_devices.size(); ++i)
        {
                const auto priority = device_priority(physical_devices[suitable_devices[i]]);
                if (priority > best_priority)
                {
                        best_priority = priority;
                        best_i = i;
                }
        }

        return suitable_devices[best_i];
}

std::size_t find_random_physical_device(const std::vector<std::size_t>& suitable_devices)
{
        ASSERT(!suitable_devices.empty());

        PCG engine;
        std::uniform_int_distribution<std::size_t> uid(0, suitable_devices.size() - 1);

        return suitable_devices[uid(engine)];
}

std::size_t find_physical_device(
        const DeviceSearchType search_type,
        const std::vector<PhysicalDevice>& physical_devices,
        const std::vector<std::size_t>& suitable_devices)
{
        if (suitable_devices.empty())
        {
                error("Failed to find a suitable Vulkan physical device");
        }

        switch (search_type)
        {
        case DeviceSearchType::BEST:
                return find_best_physical_device(physical_devices, suitable_devices);
        case DeviceSearchType::RANDOM:
                return find_random_physical_device(suitable_devices);
        }

        error("Unknown physical device search type " + to_string(enum_to_int(search_type)));
}

bool extensions_supported(const PhysicalDevice& physical_device, const std::unordered_set<std::string>& extensions)
{
        return std::ranges::all_of(
                extensions,
                [&](const std::string& e)
                {
                        return physical_device.extensions().contains(e);
                });
}

bool minimum_properties_supported(const PhysicalDevice& physical_device)
{
        const VkPhysicalDeviceLimits& limits = physical_device.info().properties.properties_10.limits;
        return limits.maxStorageBufferRange >= MIN_STORAGE_BUFFER_RANGE;
}

bool suitable_physical_device(
        const PhysicalDevice& physical_device,
        const VkSurfaceKHR surface,
        const DeviceFunctionality& device_functionality,
        const bool optional_as_required)
{
        if (!check_features(device_functionality.required_features, physical_device.features()))
        {
                return false;
        }

        if (optional_as_required && !check_features(device_functionality.optional_features, physical_device.features()))
        {
                return false;
        }

        if (!extensions_supported(physical_device, device_functionality.required_extensions))
        {
                return false;
        }

        if (optional_as_required && !extensions_supported(physical_device, device_functionality.optional_extensions))
        {
                return false;
        }

        if (!minimum_properties_supported(physical_device))
        {
                return false;
        }

        if (!physical_device.find_family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
        {
                return false;
        }

        if (surface != VK_NULL_HANDLE)
        {
                try
                {
                        static_cast<void>(physical_device.presentation_family_index());
                }
                catch (...)
                {
                        return false;
                }

                if (!surface_suitable(physical_device.device(), surface))
                {
                        return false;
                }
        }

        return true;
}

std::vector<std::size_t> suitable_physical_devices(
        const std::vector<PhysicalDevice>& physical_devices,
        const VkSurfaceKHR surface,
        const DeviceFunctionality& device_functionality,
        const bool optional_as_required)
{
        std::vector<std::size_t> res;
        for (std::size_t i = 0; i < physical_devices.size(); ++i)
        {
                if (suitable_physical_device(physical_devices[i], surface, device_functionality, optional_as_required))
                {
                        res.push_back(i);
                }
        }
        return res;
}
}

std::vector<VkPhysicalDevice> find_devices(const VkInstance instance)
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
        oss << "No Vulkan physical device found with minimum required version ";
        oss << strings::api_version_to_string(API_VERSION);
        oss << '\n';
        oss << "Found " << (all_devices.size() > 1 ? "devices" : "device");
        for (const VkPhysicalDevice device : all_devices)
        {
                oss << '\n';
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);
                oss << static_cast<const char*>(properties.deviceName) << "\n";
                oss << "  API version " << strings::api_version_to_string(properties.apiVersion);
        }

        error(oss.str());
}

PhysicalDevice find_device(
        const DeviceSearchType search_type,
        const VkInstance instance,
        const VkSurfaceKHR surface,
        const DeviceFunctionality& device_functionality)
{
        LOG(overview_physical_devices(instance, surface));

        const std::vector<VkPhysicalDevice> handles = find_devices(instance);

        std::vector<PhysicalDevice> devices;
        devices.reserve(handles.size());
        for (const VkPhysicalDevice handle : handles)
        {
                devices.emplace_back(handle, surface);
        }

        std::vector<std::size_t> suitable_devices = suitable_physical_devices(
                std::as_const(devices), surface, device_functionality, /*optional_as_required=*/true);

        if (suitable_devices.empty())
        {
                suitable_devices = suitable_physical_devices(
                        std::as_const(devices), surface, device_functionality, /*optional_as_required=*/false);
        }

        const std::size_t device = find_physical_device(search_type, devices, suitable_devices);
        ASSERT(device < devices.size());

        return std::move(devices[device]);
}
}
