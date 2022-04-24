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

#include "queues.h"

#include <src/com/error.h>
#include <src/com/print.h>

namespace ns::vulkan
{
namespace
{
std::vector<std::uint32_t> distribute_device_queues(
        const std::uint32_t count,
        const std::uint32_t family_index,
        const std::uint32_t device_queue_count,
        std::unordered_map<std::uint32_t, std::uint32_t>* const queue_count)
{
        std::vector<std::uint32_t> queues;

        for (std::size_t i = 0; i < count; ++i)
        {
                std::uint32_t& device_queue = (*queue_count)[family_index];
                if (device_queue >= device_queue_count)
                {
                        device_queue = 0;
                }
                queues.push_back(device_queue);
                ++device_queue;
        }

        return queues;
}
}

QueueDistribution distribute_queues(const PhysicalDevice& physical_device, const std::vector<QueueFamilyInfo>& data)
{
        QueueDistribution distribution;

        for (const QueueFamilyInfo& entry : data)
        {
                distribution.index_to_count[entry.index] = std::min(
                        distribution.index_to_count[entry.index] + entry.count,
                        physical_device.queue_families()[entry.index].queueCount);
        }

        distribution.device_queues.reserve(data.size());

        std::unordered_map<std::uint32_t, std::uint32_t> queue_count;

        for (const QueueFamilyInfo& entry : data)
        {
                std::vector<std::uint32_t> device_queues = distribute_device_queues(
                        entry.count, entry.index, distribution.index_to_count[entry.index], &queue_count);

                distribution.device_queues.push_back(
                        {.family_index = entry.index, .device_queues = std::move(device_queues)});
        }

        return distribution;
}

std::string device_queues_description(
        const std::vector<std::string_view>& names,
        const std::vector<QueueFamilyDevice>& device_queues)
{
        ASSERT(names.size() == device_queues.size());

        constexpr std::string_view LINE_START = "queue distribution: ";

        std::string res;
        for (std::size_t i = 0; i < names.size(); ++i)
        {
                if (!res.empty())
                {
                        res += '\n';
                }
                res += LINE_START;
                res += names[i];
                res += '\n';
                res += LINE_START;
                res += "  family = ";
                res += to_string(device_queues[i].family_index);
                res += ", queues = {";
                const std::vector<std::uint32_t>& queues = device_queues[i].device_queues;
                for (std::size_t j = 0; j < queues.size(); ++j)
                {
                        if (j != 0)
                        {
                                res += ", ";
                        }
                        res += to_string(queues[j]);
                }
                res += '}';
        }
        return res;
}

std::vector<Queue> create_device_queues(const Device& device, const QueueFamilyDevice& device_queues)
{
        std::vector<Queue> res;
        res.reserve(device_queues.device_queues.size());
        for (const auto queue : device_queues.device_queues)
        {
                res.push_back(device.queue(device_queues.family_index, queue));
        }
        return res;
}

std::unordered_map<std::uint32_t, std::vector<VkQueue>> find_device_queues(
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
