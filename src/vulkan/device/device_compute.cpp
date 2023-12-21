/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "queues.h"

#include "../physical_device/functionality.h"
#include "../physical_device/physical_device.h"

#include <src/com/error.h>
#include <src/com/log.h>

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ns::vulkan
{
namespace
{
constexpr unsigned COMPUTE_QUEUE_COUNT = 1;
constexpr unsigned TRANSFER_QUEUE_COUNT = 1;

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
}

DeviceCompute::DeviceCompute(
        const PhysicalDeviceSearchType search_type,
        const VkInstance instance,
        const DeviceFunctionality& device_functionality)
        : physical_device_(find_physical_device(search_type, instance, VK_NULL_HANDLE, device_functionality)),
          compute_family_index_(find_compute_family_index(physical_device_)),
          transfer_family_index_(find_transfer_family_index(physical_device_))
{
        constexpr int COMPUTE = 0;
        constexpr int TRANSFER = 1;

        const std::vector<QueueFamilyInfo> family_info = [&]
        {
                std::vector<QueueFamilyInfo> res(2);
                res[COMPUTE] = {.family_index = compute_family_index_, .queue_count = COMPUTE_QUEUE_COUNT};
                res[TRANSFER] = {.family_index = transfer_family_index_, .queue_count = TRANSFER_QUEUE_COUNT};
                return res;
        }();

        const QueueDistribution distribution = distribute_queues(physical_device_, family_info);

        LOG(device_queues_description({"compute", "transfer"}, distribution.device_queues));

        device_.emplace(&physical_device_, distribution.index_to_count, device_functionality);

        compute_queues_ = create_device_queues(*device_, distribution.device_queues[COMPUTE]);
        transfer_queues_ = create_device_queues(*device_, distribution.device_queues[TRANSFER]);
}
}
