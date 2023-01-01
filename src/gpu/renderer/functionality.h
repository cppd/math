/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/vulkan/device/device.h>
#include <src/vulkan/physical_device/functionality.h>

namespace ns::gpu::renderer
{
vulkan::DeviceFunctionality device_ray_tracing_functionality();
vulkan::DeviceFunctionality device_functionality();

bool ray_tracing_supported(const vulkan::Device& device);
}
