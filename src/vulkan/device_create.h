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

#include "device_info.h"
#include "objects.h"
#include "physical_device.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ns::vulkan
{
handle::Device create_device(
        const PhysicalDevice* physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
        std::vector<std::string> required_extensions,
        const DeviceFeatures& features);
}
