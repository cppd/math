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

#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
std::vector<std::uint32_t> distribute_device_queues(
        std::uint32_t count,
        const std::string_view& queue_name,
        std::uint32_t family_index,
        std::uint32_t device_queue_count,
        std::unordered_map<std::uint32_t, std::uint32_t>* queue_count,
        std::string* description);

std::unordered_map<std::uint32_t, std::vector<VkQueue>> find_device_queues(
        VkDevice device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families);
}
