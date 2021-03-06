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

        const Instance m_instance;
        const std::optional<DebugReportCallback> m_callback;

        const std::optional<SurfaceKHR> m_surface;

        const PhysicalDevice m_physical_device;

        const uint32_t m_graphics_compute_family_index;
        const uint32_t m_compute_family_index;
        const uint32_t m_transfer_family_index;
        const uint32_t m_presentation_family_index;

        const Device m_device;

        const CommandPool m_graphics_compute_command_pool;
        const CommandPool m_compute_command_pool;
        const CommandPool m_transfer_command_pool;

        std::array<Queue, GRAPHICS_COMPUTE_QUEUE_COUNT> m_graphics_compute_queues;
        std::array<Queue, COMPUTE_QUEUE_COUNT> m_compute_queues;
        std::array<Queue, TRANSFER_QUEUE_COUNT> m_transfer_queues;
        std::array<Queue, PRESENTATION_QUEUE_COUNT> m_presentation_queues;

public:
        VulkanInstance(
                const std::vector<std::string>& required_instance_extensions,
                const std::vector<std::string>& required_device_extensions,
                const std::vector<PhysicalDeviceFeatures>& required_features,
                const std::vector<PhysicalDeviceFeatures>& optional_features,
                const std::optional<std::function<VkSurfaceKHR(VkInstance)>>& create_surface = std::nullopt);

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
                ASSERT(m_surface);
                return *m_surface;
        }

        const Device& device() const
        {
                return m_device;
        }

        const vulkan::CommandPool& graphics_compute_command_pool() const
        {
                return m_graphics_compute_command_pool;
        }

        const vulkan::CommandPool& compute_command_pool() const
        {
                return m_compute_command_pool;
        }

        const vulkan::CommandPool& transfer_command_pool() const
        {
                return m_transfer_command_pool;
        }

        const std::array<Queue, GRAPHICS_COMPUTE_QUEUE_COUNT>& graphics_compute_queues() const
        {
                return m_graphics_compute_queues;
        }

        const Queue& compute_queue() const
        {
                return m_compute_queues[0];
        }

        const Queue& transfer_queue() const
        {
                return m_transfer_queues[0];
        }

        const Queue& presentation_queue() const
        {
                ASSERT(m_surface);
                return m_presentation_queues[0];
        }
};
}
