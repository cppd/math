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

#include "info.h"

#include <src/com/error.h>
#include <src/vulkan/error.h>
#include <src/vulkan/extensions.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace ns::vulkan::physical_device
{
namespace
{
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
}

PhysicalDevice::PhysicalDevice(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface)
        : physical_device_(physical_device),
          device_info_(device_info(physical_device)),
          presentation_support_(find_queue_family_presentation_support(surface, physical_device_))
{
        ASSERT(physical_device_ != VK_NULL_HANDLE);
        ASSERT(device_info_.queue_families.size() == presentation_support_.size());
}

const DeviceInfo& PhysicalDevice::info() const
{
        return device_info_;
}

VkPhysicalDevice PhysicalDevice::device() const
{
        return physical_device_;
}

const std::unordered_set<std::string>& PhysicalDevice::extensions() const
{
        return device_info_.extensions;
}

const Properties& PhysicalDevice::properties() const
{
        return device_info_.properties;
}

const Features& PhysicalDevice::features() const
{
        return device_info_.features;
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::queue_families() const
{
        return device_info_.queue_families;
}

std::optional<std::uint32_t> PhysicalDevice::find_family_index(
        const VkQueueFlags present_flags,
        const VkQueueFlags absent_flags) const
{
        return find_family(device_info_.queue_families, present_flags, absent_flags);
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
}
