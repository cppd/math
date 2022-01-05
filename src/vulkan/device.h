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

#include "device_info.h"
#include "objects.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
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

        std::uint32_t family_index(
                VkQueueFlags set_flags,
                VkQueueFlags not_set_flags,
                const std::vector<VkQueueFlags>& default_flags) const;
        std::uint32_t presentation_family_index() const;
        bool supports_extensions(const std::vector<std::string>& extensions) const;
        bool queue_family_supports_presentation(std::uint32_t index) const;
};

class Device final
{
        handle::Device device_;
        const PhysicalDevice* physical_device_ = nullptr;
        DeviceFeatures features_;
        std::unordered_map<std::uint32_t, std::vector<VkQueue>> queues_;

public:
        Device(const PhysicalDevice* physical_device, const VkDeviceCreateInfo& create_info);

        operator VkDevice() const& noexcept
        {
                return device_;
        }
        operator VkDevice() const&& = delete;

        VkPhysicalDevice physical_device() const;

        const DeviceFeatures& features() const;
        const DeviceProperties& properties() const;

        Queue queue(std::uint32_t family_index, std::uint32_t queue_index) const;
};

PhysicalDevice create_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& required_features);

Device create_device(
        const PhysicalDevice* physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& required_features,
        const DeviceFeatures& optional_features);
}
