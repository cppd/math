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

#pragma once

#include "objects.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
std::vector<VkPhysicalDevice> physical_devices(VkInstance instance);

class PhysicalDevice final
{
        VkPhysicalDevice physical_device_;
        DeviceFeatures features_;
        DeviceProperties properties_;
        std::vector<VkQueueFamilyProperties> queue_families_;
        std::vector<bool> presentation_supported_;
        std::unordered_set<std::string> supported_extensions_;

public:
        PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

        VkPhysicalDevice device() const;

        const DeviceFeatures& features() const;
        const DeviceProperties& properties() const;
        const std::vector<VkQueueFamilyProperties>& queue_families() const;
        const std::unordered_set<std::string>& supported_extensions() const;

        uint32_t family_index(
                VkQueueFlags set_flags,
                VkQueueFlags not_set_flags,
                const std::vector<VkQueueFlags>& default_flags) const;
        uint32_t presentation_family_index() const;
        bool supports_extensions(const std::vector<std::string>& extensions) const;
        bool queue_family_supports_presentation(uint32_t index) const;
};

PhysicalDevice create_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& required_features);

Device create_device(
        const PhysicalDevice& physical_device,
        const std::unordered_map<uint32_t, uint32_t>& queue_families,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& required_features,
        const DeviceFeatures& optional_features);
}
