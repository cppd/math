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

#include "physical_device.h"

#include "features.h"
#include "functionality.h"
#include "info.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/vulkan/api_version.h>
#include <src/vulkan/error.h>
#include <src/vulkan/extensions.h>
#include <src/vulkan/overview.h>
#include <src/vulkan/strings.h>
#include <src/vulkan/surface.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
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

std::optional<std::uint32_t> find_family(
        const std::vector<VkQueueFamilyProperties>& families,
        const VkQueueFlags present_flags,
        const VkQueueFlags absent_flags)
{
        if (!present_flags)
        {
                error("No present flags specified for finding queue family index");
        }

        if (present_flags & absent_flags)
        {
                error("Flag intersection for finding queue family index");
        }

        for (std::size_t i = 0; i < families.size(); ++i)
        {
                const VkQueueFamilyProperties& p = families[i];

                if (p.queueCount < 1)
                {
                        continue;
                }

                if (((p.queueFlags & present_flags) == present_flags) && !(p.queueFlags & absent_flags))
                {
                        return i;
                }
        }

        return std::nullopt;
}

std::vector<bool> find_queue_family_presentation_support(const VkSurfaceKHR surface, const VkPhysicalDevice device)
{
        std::uint32_t family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);

        std::vector<bool> res(family_count, false);

        if (surface == VK_NULL_HANDLE)
        {
                return res;
        }

        for (std::uint32_t family_index = 0; family_index < family_count; ++family_index)
        {
                VkBool32 supported;
                VULKAN_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, family_index, surface, &supported));

                res[family_index] = (supported == VK_TRUE);
        }

        return res;
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
        const PhysicalDeviceSearchType search_type,
        const std::vector<PhysicalDevice>& physical_devices,
        const std::vector<std::size_t>& suitable_devices)
{
        if (suitable_devices.empty())
        {
                error("Failed to find a suitable Vulkan physical device");
        }

        switch (search_type)
        {
        case PhysicalDeviceSearchType::BEST:
                return find_best_physical_device(physical_devices, suitable_devices);
        case PhysicalDeviceSearchType::RANDOM:
                return find_random_physical_device(suitable_devices);
        }

        error("Unknown physical device search type " + to_string(enum_to_int(search_type)));
}

bool extensions_supported(const PhysicalDevice& physical_device, const std::unordered_set<std::string>& extensions)
{
        return std::all_of(
                extensions.cbegin(), extensions.cend(),
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

std::optional<std::uint32_t> PhysicalDevice::find_family_index(
        const VkQueueFlags present_flags,
        const VkQueueFlags absent_flags) const
{
        return find_family(info_.queue_families, present_flags, absent_flags);
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

PhysicalDevice find_physical_device(
        const PhysicalDeviceSearchType search_type,
        const VkInstance instance,
        const VkSurfaceKHR surface,
        const DeviceFunctionality& device_functionality)
{
        LOG(overview_physical_devices(instance, surface));

        const std::vector<VkPhysicalDevice> handles = find_physical_devices(instance);

        std::vector<PhysicalDevice> devices;
        devices.reserve(handles.size());
        for (const VkPhysicalDevice handle : handles)
        {
                devices.emplace_back(handle, surface);
        }

        std::vector<std::size_t> suitable_devices = suitable_physical_devices(
                std::as_const(devices), surface, device_functionality, true /*optional_as_required*/);

        if (suitable_devices.empty())
        {
                suitable_devices = suitable_physical_devices(
                        std::as_const(devices), surface, device_functionality, false /*optional_as_required*/);
        }

        const std::size_t device = find_physical_device(search_type, devices, suitable_devices);
        ASSERT(device < devices.size());

        return std::move(devices[device]);
}
}
