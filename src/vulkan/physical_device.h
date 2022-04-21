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

#pragma once

#include "functionality.h"
#include "objects.h"
#include "physical_device_info.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
class PhysicalDevice final
{
        VkPhysicalDevice physical_device_;
        PhysicalDeviceInfo info_;
        std::vector<bool> presentation_support_;

public:
        PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

        VkPhysicalDevice device() const;

        const PhysicalDeviceInfo& info() const;
        const std::unordered_set<std::string>& extensions() const;
        const PhysicalDeviceProperties& properties() const;
        const PhysicalDeviceFeatures& features() const;
        const std::vector<VkQueueFamilyProperties>& queue_families() const;

        std::uint32_t find_family_index(
                VkQueueFlags set_flags,
                VkQueueFlags not_set_flags,
                const std::vector<VkQueueFlags>& default_flags) const;

        std::uint32_t presentation_family_index() const;

        bool queue_family_supports_presentation(std::uint32_t index) const;
};

std::vector<VkPhysicalDevice> find_physical_devices(VkInstance instance);

enum class PhysicalDeviceSearchType
{
        BEST,
        RANDOM
};

PhysicalDevice find_physical_device(
        PhysicalDeviceSearchType search_type,
        VkInstance instance,
        VkSurfaceKHR surface,
        const DeviceFunctionality& device_functionality);
}
