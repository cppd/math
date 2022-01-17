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

#include "device.h"
#include "extensions.h"
#include "functionality.h"
#include "objects.h"
#include "physical_device.h"

#include <src/com/error.h>

#include <array>
#include <optional>

namespace ns::vulkan
{
class DeviceInstance final
{
        static constexpr unsigned GRAPHICS_COMPUTE_QUEUE_COUNT = 2;
        static constexpr unsigned COMPUTE_QUEUE_COUNT = 1;
        static constexpr unsigned TRANSFER_QUEUE_COUNT = 1;
        static constexpr unsigned PRESENTATION_QUEUE_COUNT = 1;

        const PhysicalDevice physical_device_;

        const std::uint32_t graphics_compute_family_index_;
        const std::uint32_t compute_family_index_;
        const std::uint32_t transfer_family_index_;
        const std::uint32_t presentation_family_index_;

        const Device device_;
        const std::optional<DeviceExtensionFunctions> device_extension_functions_;

        std::array<Queue, GRAPHICS_COMPUTE_QUEUE_COUNT> graphics_compute_queues_;
        std::array<Queue, COMPUTE_QUEUE_COUNT> compute_queues_;
        std::array<Queue, TRANSFER_QUEUE_COUNT> transfer_queues_;
        std::array<Queue, PRESENTATION_QUEUE_COUNT> presentation_queues_;

public:
        explicit DeviceInstance(
                VkInstance instance,
                const DeviceFunctionality& device_functionality = {},
                VkSurfaceKHR surface = VK_NULL_HANDLE);

        ~DeviceInstance() = default;

        DeviceInstance(const DeviceInstance&) = delete;
        DeviceInstance(DeviceInstance&&) = delete;
        DeviceInstance& operator=(const DeviceInstance&) = delete;
        DeviceInstance& operator=(DeviceInstance&&) = delete;

        const Device& device() const
        {
                return device_;
        }

        std::uint32_t graphics_compute_family_index() const
        {
                return graphics_compute_family_index_;
        }

        std::uint32_t compute_family_index() const
        {
                return compute_family_index_;
        }

        std::uint32_t transfer_family_index() const
        {
                return transfer_family_index_;
        }

        const std::array<Queue, GRAPHICS_COMPUTE_QUEUE_COUNT>& graphics_compute_queues() const
        {
                return graphics_compute_queues_;
        }

        const Queue& compute_queue() const
        {
                return compute_queues_[0];
        }

        const Queue& transfer_queue() const
        {
                return transfer_queues_[0];
        }

        const Queue& presentation_queue() const
        {
                ASSERT(static_cast<VkQueue>(presentation_queues_[0]) != VK_NULL_HANDLE);
                return presentation_queues_[0];
        }
};
}
