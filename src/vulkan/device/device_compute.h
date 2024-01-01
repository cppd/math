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

#pragma once

#include "device.h"

#include "../objects.h"
#include "../physical_device/functionality.h"
#include "../physical_device/physical_device.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace ns::vulkan
{
class DeviceCompute final
{
        const PhysicalDevice physical_device_;

        const std::uint32_t compute_family_index_;
        const std::uint32_t transfer_family_index_;

        std::optional<Device> device_;

        std::vector<Queue> compute_queues_;
        std::vector<Queue> transfer_queues_;

public:
        DeviceCompute(
                PhysicalDeviceSearchType search_type,
                VkInstance instance,
                const DeviceFunctionality& device_functionality);

        DeviceCompute(const DeviceCompute&) = delete;
        DeviceCompute(DeviceCompute&&) = delete;
        DeviceCompute& operator=(const DeviceCompute&) = delete;
        DeviceCompute& operator=(DeviceCompute&&) = delete;

        [[nodiscard]] const Device& device() const
        {
                return *device_;
        }

        [[nodiscard]] std::uint32_t compute_family_index() const
        {
                return compute_family_index_;
        }

        [[nodiscard]] std::uint32_t transfer_family_index() const
        {
                return transfer_family_index_;
        }

        [[nodiscard]] const Queue& compute_queue() const
        {
                return compute_queues_[0];
        }

        [[nodiscard]] const Queue& transfer_queue() const
        {
                return transfer_queues_[0];
        }
};
}
