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

#include "device_graphics.h"

#include "device/queues.h"

#include <src/com/log.h>

#include <string>
#include <unordered_map>

namespace ns::vulkan
{
namespace
{
std::uint32_t find_graphics_compute_family_index(const PhysicalDevice& device)
{
        if (const auto index = device.find_family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
        {
                return *index;
        }

        error("Graphics compute queue family index not found");
}

std::uint32_t find_compute_family_index(const PhysicalDevice& device)
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

std::uint32_t find_transfer_family_index(const PhysicalDevice& device)
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

std::unordered_map<std::uint32_t, std::uint32_t> compute_device_queue_count(
        const PhysicalDevice& physical_device,
        const std::uint32_t graphics_compute_index,
        const std::uint32_t graphics_compute_count,
        const std::uint32_t compute_index,
        const std::uint32_t compute_count,
        const std::uint32_t transfer_index,
        const std::uint32_t transfer_count,
        const std::uint32_t presentation_index,
        const std::uint32_t presentation_count)
{
        std::unordered_map<std::uint32_t, std::uint32_t> queues;

        const auto compute = [&](const auto index, const auto count)
        {
                queues[index] = std::min(queues[index] + count, physical_device.queue_families()[index].queueCount);
        };

        compute(graphics_compute_index, graphics_compute_count);
        compute(compute_index, compute_count);
        compute(transfer_index, transfer_count);
        compute(presentation_index, presentation_count);

        return queues;
}
}

DeviceGraphics::DeviceGraphics(
        const VkInstance instance,
        const DeviceFunctionality& device_functionality,
        const VkSurfaceKHR surface)
        : physical_device_(
                find_physical_device(PhysicalDeviceSearchType::BEST, instance, surface, device_functionality)),
          graphics_compute_family_index_(find_graphics_compute_family_index(physical_device_)),
          compute_family_index_(find_compute_family_index(physical_device_)),
          transfer_family_index_(find_transfer_family_index(physical_device_)),
          presentation_family_index_(physical_device_.presentation_family_index()),
          device_(&physical_device_,
                  compute_device_queue_count(
                          physical_device_,
                          graphics_compute_family_index_,
                          GRAPHICS_COMPUTE_QUEUE_COUNT,
                          compute_family_index_,
                          COMPUTE_QUEUE_COUNT,
                          transfer_family_index_,
                          TRANSFER_QUEUE_COUNT,
                          presentation_family_index_,
                          PRESENTATION_QUEUE_COUNT),
                  device_functionality),
          device_extension_functions_(device_)
{
        std::string description;
        std::unordered_map<std::uint32_t, std::uint32_t> queue_count;

        distribute_device_queues(
                device_, "graphics compute", graphics_compute_family_index_, graphics_compute_queues_, &queue_count,
                &description);

        distribute_device_queues(
                device_, "compute", compute_family_index_, compute_queues_, &queue_count, &description);

        distribute_device_queues(
                device_, "transfer", transfer_family_index_, transfer_queues_, &queue_count, &description);

        distribute_device_queues(
                device_, "presentation", presentation_family_index_, presentation_queues_, &queue_count, &description);

        LOG(description);
}
}
