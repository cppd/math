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

#include "device.h"

#include "device_create.h"
#include "features.h"

#include <src/com/error.h>
#include <src/com/print.h>

namespace ns::vulkan
{
namespace
{
std::unordered_map<std::uint32_t, std::vector<VkQueue>> find_queues(
        const VkDevice device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families)
{
        std::unordered_map<std::uint32_t, std::vector<VkQueue>> queues;

        for (const auto& [family_index, queue_count] : queue_families)
        {
                const auto [iter, inserted] = queues.try_emplace(family_index);

                if (!inserted)
                {
                        error("Non unique device queue family indices");
                }

                for (std::uint32_t queue_index = 0; queue_index < queue_count; ++queue_index)
                {
                        VkQueue queue;
                        vkGetDeviceQueue(device, family_index, queue_index, &queue);

                        if (queue == VK_NULL_HANDLE)
                        {
                                error("Null queue handle, family " + to_string(family_index) + ", queue "
                                      + to_string(queue_index));
                        }

                        iter->second.push_back(queue);
                }
        }

        return queues;
}
}

Device::Device(
        const PhysicalDevice* const physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
        const std::vector<std::string>& required_extensions,
        const DeviceFeatures& required_features,
        const DeviceFeatures& optional_features)
        : physical_device_(physical_device),
          features_(make_features(required_features, optional_features, physical_device_->features())),
          device_(create_device(physical_device_, queue_families, required_extensions, features_)),
          queues_(find_queues(device_, queue_families))
{
}

VkPhysicalDevice Device::physical_device() const
{
        return physical_device_->device();
}

const DeviceFeatures& Device::features() const
{
        return features_;
}

const DeviceProperties& Device::properties() const
{
        return physical_device_->properties();
}

Queue Device::queue(const std::uint32_t family_index, const std::uint32_t queue_index) const
{
        const auto iter = queues_.find(family_index);

        if (iter == queues_.cend())
        {
                error("Queue family index " + to_string(family_index) + " not found");
        }

        if (queue_index >= iter->second.size())
        {
                error("Queue " + to_string(queue_index) + " not found");
        }

        return {family_index, iter->second[queue_index]};
}
}
