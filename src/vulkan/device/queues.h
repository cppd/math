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

#pragma once

#include "../device.h"
#include "../physical_device.h"

#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace ns::vulkan
{
std::unordered_map<std::uint32_t, std::uint32_t> compute_device_queue_count(
        const PhysicalDevice& physical_device,
        const std::vector<std::tuple<std::uint32_t, std::uint32_t>>& family_index_and_count);

void distribute_device_queues(
        const Device& device,
        const std::string_view& queue_name,
        std::uint32_t family_index,
        const std::span<Queue>& queues,
        std::unordered_map<std::uint32_t, std::uint32_t>* queue_count,
        std::string* description);

std::unordered_map<std::uint32_t, std::vector<VkQueue>> find_device_queues(
        const VkDevice device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families);
}
