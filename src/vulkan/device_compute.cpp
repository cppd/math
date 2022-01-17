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

#include "device_queues.h"

#include <src/com/log.h>

#include <string>
#include <unordered_map>

namespace ns::vulkan
{
DeviceCompute::DeviceCompute(
        const PhysicalDeviceSearchType search_type,
        const VkInstance instance,
        const DeviceFunctionality& device_functionality)
        : physical_device_(find_physical_device(search_type, instance, VK_NULL_HANDLE, device_functionality)),
          compute_family_index_(
                  physical_device_.family_index(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, {VK_QUEUE_COMPUTE_BIT})),
          transfer_family_index_(physical_device_.family_index(
                  VK_QUEUE_TRANSFER_BIT,
                  VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
                  // All commands that are allowed on a queue that supports
                  // transfer operations are also allowed on a queue that
                  // supports either graphics or compute operations
                  {VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT})),
          device_(&physical_device_,
                  compute_device_queue_count(
                          physical_device_,
                          {{compute_family_index_, COMPUTE_QUEUE_COUNT},
                           {transfer_family_index_, TRANSFER_QUEUE_COUNT}}),
                  device_functionality)
{
        std::string description;
        std::unordered_map<std::uint32_t, std::uint32_t> queue_count;

        distribute_device_queues(
                device_, "compute", compute_family_index_, compute_queues_, &queue_count, &description);

        distribute_device_queues(
                device_, "transfer", transfer_family_index_, transfer_queues_, &queue_count, &description);

        LOG(description);
}
}
