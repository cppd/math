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

#include "functionality.h"
#include "physical_device.h"

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::vulkan::physical_device
{
std::vector<VkPhysicalDevice> find_devices(VkInstance instance);

enum class DeviceSearchType
{
        BEST,
        RANDOM
};

PhysicalDevice find_device(
        DeviceSearchType search_type,
        VkInstance instance,
        VkSurfaceKHR surface,
        const DeviceFunctionality& device_functionality);
}
