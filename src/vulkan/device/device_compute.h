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

#include <src/com/error.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>
#include <src/vulkan/physical_device/physical_device.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace ns::vulkan::device
{
class DeviceCompute final
{
        const physical_device::PhysicalDevice physical_device_;

        const std::uint32_t compute_family_index_;
        const std::uint32_t transfer_family_index_;

        std::optional<Device> device_;

        std::vector<Queue> compute_queues_;
        std::vector<Queue> transfer_queues_;

public:
        DeviceCompute(
                physical_device::DeviceSearchType search_type,
                VkInstance instance,
                const physical_device::DeviceFunctionality& device_functionality);

        DeviceCompute(const DeviceCompute&) = delete;
        DeviceCompute(DeviceCompute&&) = delete;
        DeviceCompute& operator=(const DeviceCompute&) = delete;
        DeviceCompute& operator=(DeviceCompute&&) = delete;

        [[nodiscard]] const Device& device() const
        {
                ASSERT(device_);
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
