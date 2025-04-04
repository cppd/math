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

#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/info.h>
#include <src/vulkan/physical_device/physical_device.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ns::vulkan::device
{
handle::Device create_device(
        const physical_device::PhysicalDevice* physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
        const std::unordered_set<std::string>& required_extensions,
        const physical_device::Features& required_features);
}
