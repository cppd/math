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

#include "device.h"
#include "objects.h"

#include <src/com/error.h>

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ns::vulkan
{
class VulkanInstance final
{
        static constexpr unsigned GRAPHICS_COMPUTE_QUEUE_COUNT = 2;
        static constexpr unsigned COMPUTE_QUEUE_COUNT = 1;
        static constexpr unsigned TRANSFER_QUEUE_COUNT = 1;
        static constexpr unsigned PRESENTATION_QUEUE_COUNT = 1;

        const Instance instance_;
        const std::optional<handle::DebugReportCallbackEXT> callback_;

        const std::optional<handle::SurfaceKHR> surface_;

        const PhysicalDevice physical_device_;

        const std::uint32_t graphics_compute_family_index_;
        const std::uint32_t compute_family_index_;
        const std::uint32_t transfer_family_index_;
        const std::uint32_t presentation_family_index_;

        const Device device_;

        const CommandPool graphics_compute_command_pool_;
        const CommandPool compute_command_pool_;
        const CommandPool transfer_command_pool_;

        std::array<Queue, GRAPHICS_COMPUTE_QUEUE_COUNT> graphics_compute_queues_;
        std::array<Queue, COMPUTE_QUEUE_COUNT> compute_queues_;
        std::array<Queue, TRANSFER_QUEUE_COUNT> transfer_queues_;
        std::array<Queue, PRESENTATION_QUEUE_COUNT> presentation_queues_;

public:
        VulkanInstance(
                const std::vector<std::string>& required_instance_extensions,
                const std::vector<std::string>& required_device_extensions,
                const DeviceFeatures& required_device_features,
                const DeviceFeatures& optional_device_features,
                const std::function<VkSurfaceKHR(VkInstance)>& create_surface = nullptr);

        ~VulkanInstance();

        VulkanInstance(const VulkanInstance&) = delete;
        VulkanInstance(VulkanInstance&&) = delete;
        VulkanInstance& operator=(const VulkanInstance&) = delete;
        VulkanInstance& operator=(VulkanInstance&&) = delete;

        //

        void device_wait_idle() const;
        void device_wait_idle_noexcept(const char* msg) const noexcept;

        //

        VkSurfaceKHR surface() const
        {
                ASSERT(surface_);
                return *surface_;
        }

        const Device& device() const
        {
                return device_;
        }

        const CommandPool& graphics_compute_command_pool() const
        {
                return graphics_compute_command_pool_;
        }

        const CommandPool& compute_command_pool() const
        {
                return compute_command_pool_;
        }

        const CommandPool& transfer_command_pool() const
        {
                return transfer_command_pool_;
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
                ASSERT(surface_);
                return presentation_queues_[0];
        }
};
}
