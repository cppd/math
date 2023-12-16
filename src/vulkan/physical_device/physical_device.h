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

#pragma once

#include "functionality.h"
#include "info.h"

#include <cstdint>
#include <optional>
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

        [[nodiscard]] VkPhysicalDevice device() const;

        [[nodiscard]] const PhysicalDeviceInfo& info() const;
        [[nodiscard]] const std::unordered_set<std::string>& extensions() const;
        [[nodiscard]] const PhysicalDeviceProperties& properties() const;
        [[nodiscard]] const PhysicalDeviceFeatures& features() const;
        [[nodiscard]] const std::vector<VkQueueFamilyProperties>& queue_families() const;

        [[nodiscard]] std::optional<std::uint32_t> find_family_index(
                VkQueueFlags present_flags,
                VkQueueFlags absent_flags = 0) const;

        [[nodiscard]] std::uint32_t presentation_family_index() const;

        [[nodiscard]] bool queue_family_supports_presentation(std::uint32_t index) const;
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
