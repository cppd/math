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

#include "queues.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/physical_device.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ns::vulkan::device
{
namespace
{
std::vector<std::uint32_t> distribute_device_queues(
        const std::uint32_t count,
        const std::uint32_t family_index,
        const std::uint32_t device_queue_count,
        std::unordered_map<std::uint32_t, std::uint32_t>* const queue_count)
{
        std::vector<std::uint32_t> res;
        res.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i)
        {
                std::uint32_t& device_queue = (*queue_count)[family_index];
                if (device_queue >= device_queue_count)
                {
                        device_queue = 0;
                }
                res.push_back(device_queue);
                ++device_queue;
        }
        return res;
}

std::map<std::uint32_t, std::map<std::uint32_t, std::string>> create_queue_description_map(
        const std::vector<std::string_view>& names,
        const std::vector<QueueFamilyDevice>& device_queues)
{
        if (names.size() != device_queues.size())
        {
                error("Names size " + to_string(names.size()) + " is not equal to device queues size "
                      + to_string(device_queues.size()));
        }

        std::map<std::uint32_t, std::map<std::uint32_t, std::string>> res;

        for (std::size_t i = 0; i < names.size(); ++i)
        {
                const auto family_index = device_queues[i].family_index;
                for (const auto queue : device_queues[i].device_queues)
                {
                        std::string& s = res[family_index][queue];
                        if (!s.empty())
                        {
                                s += ", ";
                        }
                        s += names[i];
                }
        }

        return res;
}

std::string create_queue_description_string(
        const std::string_view line_start,
        const std::map<std::uint32_t, std::map<std::uint32_t, std::string>>& queue_info)
{
        std::string res;

        for (const auto& [family_index, queues] : queue_info)
        {
                for (const auto& [queue, s] : queues)
                {
                        if (!res.empty())
                        {
                                res += '\n';
                        }
                        res += line_start;
                        res += "family index = ";
                        res += to_string(family_index);
                        res += ", queue = ";
                        res += to_string(queue);
                        res += ": ";
                        res += s;
                }
        }

        return res;
}
}

QueueDistribution distribute_queues(
        const physical_device::PhysicalDevice& physical_device,
        const std::vector<QueueFamilyInfo>& infos)
{
        QueueDistribution res;

        for (const QueueFamilyInfo& info : infos)
        {
                res.index_to_count[info.family_index] = std::min(
                        res.index_to_count[info.family_index] + info.queue_count,
                        physical_device.queue_families()[info.family_index].queueCount);
        }

        res.device_queues.reserve(infos.size());

        std::unordered_map<std::uint32_t, std::uint32_t> queue_count;

        for (const QueueFamilyInfo& info : infos)
        {
                std::vector<std::uint32_t> device_queues = distribute_device_queues(
                        info.queue_count, info.family_index, res.index_to_count[info.family_index], &queue_count);

                res.device_queues.push_back(
                        {.family_index = info.family_index, .device_queues = std::move(device_queues)});
        }

        return res;
}

std::string queues_to_string(
        const std::vector<std::string_view>& names,
        const std::vector<QueueFamilyDevice>& device_queues)
{
        return create_queue_description_string(
                "queue distribution: ", create_queue_description_map(names, device_queues));
}

std::vector<Queue> create_queues(const Device& device, const QueueFamilyDevice& device_queues)
{
        std::vector<Queue> res;
        res.reserve(device_queues.device_queues.size());
        for (const auto queue : device_queues.device_queues)
        {
                res.push_back(device.queue(device_queues.family_index, queue));
        }
        return res;
}

std::unordered_map<std::uint32_t, std::vector<VkQueue>> find_queues(
        const VkDevice device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families)
{
        std::unordered_map<std::uint32_t, std::vector<VkQueue>> res;

        for (const auto& [family_index, queue_count] : queue_families)
        {
                const auto [iter, inserted] = res.try_emplace(family_index);

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

        return res;
}
}
