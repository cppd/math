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
std::vector<std::uint32_t> distribute_device_queues(
        const std::uint32_t count,
        const std::string_view& queue_name,
        const std::uint32_t family_index,
        const std::uint32_t device_queue_count,
        std::unordered_map<std::uint32_t, std::uint32_t>* const queue_count,
        std::string* const description)
{
        if (!description->empty())
        {
                *description += '\n';
        }

        *description += queue_name;
        *description += " queues, family index = " + to_string(family_index);

        std::vector<std::uint32_t> queues;

        for (std::size_t i = 0; i < count; ++i)
        {
                std::uint32_t& device_queue = (*queue_count)[family_index];
                if (device_queue >= device_queue_count)
                {
                        device_queue = 0;
                }
                queues.push_back(device_queue);
                *description += "\n  queue = " + to_string(i) + ", device queue = " + to_string(device_queue);
                ++device_queue;
        }

        return queues;
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
