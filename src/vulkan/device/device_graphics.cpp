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

#include "device_graphics.h"

#include "queues.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/vulkan/physical_device/functionality.h>
#include <src/vulkan/physical_device/physical_device.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::vulkan
{
namespace
{
constexpr unsigned GRAPHICS_COMPUTE_QUEUE_COUNT = 2;
constexpr unsigned COMPUTE_QUEUE_COUNT = 1;
constexpr unsigned TRANSFER_QUEUE_COUNT = 1;
constexpr unsigned PRESENTATION_QUEUE_COUNT = 1;

std::uint32_t find_graphics_compute_family_index(const physical_device::PhysicalDevice& device)
{
        if (const auto index = device.find_family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
        {
                return *index;
        }

        error("Graphics compute queue family index not found");
}

std::uint32_t find_compute_family_index(const physical_device::PhysicalDevice& device)
{
        if (const auto index = device.find_family_index(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT))
        {
                return *index;
        }

        if (const auto index = device.find_family_index(VK_QUEUE_COMPUTE_BIT))
        {
                return *index;
        }

        error("Compute queue family index not found");
}

std::uint32_t find_transfer_family_index(const physical_device::PhysicalDevice& device)
{
        if (const auto index =
                    device.find_family_index(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
        {
                return *index;
        }

        // All commands that are allowed on a queue that supports
        // transfer operations are also allowed on a queue that
        // supports either graphics or compute operations
        for (const VkQueueFlagBits bit : {VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT})
        {
                if (const auto index = device.find_family_index(bit))
                {
                        return *index;
                }
        }

        error("Transfer queue family not found");
}
}

DeviceGraphics::DeviceGraphics(
        const VkInstance instance,
        const physical_device::DeviceFunctionality& device_functionality,
        const VkSurfaceKHR surface)
        : physical_device_(
                  find_device(physical_device::DeviceSearchType::BEST, instance, surface, device_functionality)),
          graphics_compute_family_index_(find_graphics_compute_family_index(physical_device_)),
          compute_family_index_(find_compute_family_index(physical_device_)),
          transfer_family_index_(find_transfer_family_index(physical_device_)),
          presentation_family_index_(physical_device_.presentation_family_index())
{
        constexpr int GRAPHICS_COMPUTE = 0;
        constexpr int COMPUTE = 1;
        constexpr int TRANSFER = 2;
        constexpr int PRESENTATION = 3;

        const std::vector<QueueFamilyInfo> family_info = [&]
        {
                std::vector<QueueFamilyInfo> res(4);
                res[GRAPHICS_COMPUTE] = {
                        .family_index = graphics_compute_family_index_,
                        .queue_count = GRAPHICS_COMPUTE_QUEUE_COUNT};
                res[COMPUTE] = {.family_index = compute_family_index_, .queue_count = COMPUTE_QUEUE_COUNT};
                res[TRANSFER] = {.family_index = transfer_family_index_, .queue_count = TRANSFER_QUEUE_COUNT};
                res[PRESENTATION] = {
                        .family_index = presentation_family_index_,
                        .queue_count = PRESENTATION_QUEUE_COUNT};
                return res;
        }();

        const QueueDistribution distribution = distribute_queues(physical_device_, family_info);

        LOG(device_queues_description(
                {"graphics compute", "compute", "transfer", "presentation"}, distribution.device_queues));

        device_.emplace(&physical_device_, distribution.index_to_count, device_functionality);

        device_extension_functions_.emplace(device_->handle());

        graphics_compute_queues_ = create_device_queues(*device_, distribution.device_queues[GRAPHICS_COMPUTE]);
        compute_queues_ = create_device_queues(*device_, distribution.device_queues[COMPUTE]);
        transfer_queues_ = create_device_queues(*device_, distribution.device_queues[TRANSFER]);
        presentation_queues_ = create_device_queues(*device_, distribution.device_queues[PRESENTATION]);
}
}
