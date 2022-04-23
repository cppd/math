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

#include "device_compute.h"

#include "device/queues.h"

#include <src/com/log.h>

#include <string>
#include <unordered_map>

namespace ns::vulkan
{
namespace
{
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
        const std::uint32_t compute_index,
        const std::uint32_t compute_count,
        const std::uint32_t transfer_index,
        const std::uint32_t transfer_count)
{
        std::unordered_map<std::uint32_t, std::uint32_t> queues;

        const auto compute = [&](const auto index, const auto count)
        {
                queues[index] = std::min(queues[index] + count, physical_device.queue_families()[index].queueCount);
        };

        compute(compute_index, compute_count);
        compute(transfer_index, transfer_count);

        return queues;
}
}

DeviceCompute::DeviceCompute(
        const PhysicalDeviceSearchType search_type,
        const VkInstance instance,
        const DeviceFunctionality& device_functionality)
        : physical_device_(find_physical_device(search_type, instance, VK_NULL_HANDLE, device_functionality)),
          compute_family_index_(find_compute_family_index(physical_device_)),
          transfer_family_index_(find_transfer_family_index(physical_device_)),
          device_(&physical_device_,
                  compute_device_queue_count(
                          physical_device_,
                          compute_family_index_,
                          COMPUTE_QUEUE_COUNT,
                          transfer_family_index_,
                          TRANSFER_QUEUE_COUNT),
                  device_functionality)
{
        std::string description;
        std::unordered_map<std::uint32_t, std::uint32_t> queue_count;

        for (std::size_t i = 0;
             const auto queue_index : distribute_device_queues(
                     COMPUTE_QUEUE_COUNT, "compute", compute_family_index_, device_.queue_count(compute_family_index_),
                     &queue_count, &description))
        {
                compute_queues_[i++] = device_.queue(compute_family_index_, queue_index);
        }

        for (std::size_t i = 0;
             const auto queue_index : distribute_device_queues(
                     TRANSFER_QUEUE_COUNT, "transfer", transfer_family_index_,
                     device_.queue_count(transfer_family_index_), &queue_count, &description))
        {
                transfer_queues_[i++] = device_.queue(transfer_family_index_, queue_index);
        }

        LOG(description);
}
}
