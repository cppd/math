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

#include <src/vulkan/device.h>
#include <src/vulkan/instance.h>
#include <src/window/handle.h>

#include <memory>

namespace ns::view
{
std::unique_ptr<vulkan::VulkanInstance> create_instance(
        window::WindowID window,
        std::vector<std::string> required_device_extensions,
        const std::vector<std::string>& optional_device_extensions,
        const vulkan::PhysicalDeviceFeatures& required_device_features,
        const vulkan::PhysicalDeviceFeatures& optional_device_features);
}
