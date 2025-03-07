/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/physical_device.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ns::vulkan::device
{
struct QueueFamilyInfo final
{
        std::uint32_t family_index;
        std::uint32_t queue_count;
};

struct QueueFamilyDevice final
{
        std::uint32_t family_index;
        std::vector<std::uint32_t> device_queues;
};

struct QueueDistribution final
{
        std::unordered_map<std::uint32_t, std::uint32_t> index_to_count;
        std::vector<QueueFamilyDevice> device_queues;
};

QueueDistribution distribute_queues(
        const physical_device::PhysicalDevice& physical_device,
        const std::vector<QueueFamilyInfo>& infos);

std::string queues_to_string(
        const std::vector<std::string_view>& names,
        const std::vector<QueueFamilyDevice>& device_queues);

std::vector<Queue> create_queues(const Device& device, const QueueFamilyDevice& device_queues);

std::unordered_map<std::uint32_t, std::vector<VkQueue>> find_queues(
        VkDevice device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families);
}
