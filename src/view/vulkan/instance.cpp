/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "instance.h"

#include <src/window/surface.h>

namespace ns::view
{
std::unique_ptr<vulkan::VulkanInstance> create_instance(
        const window::WindowID& window,
        const vulkan::DeviceFeatures& required_device_features)
{
        const std::vector<std::string> instance_extensions = window::vulkan_create_surface_required_extensions();

        const std::vector<std::string> device_extensions = {};

        const vulkan::DeviceFeatures optional_device_features = {};

        const std::function<VkSurfaceKHR(VkInstance)> surface_function = [&](const VkInstance instance)
        {
                return window::vulkan_create_surface(window, instance);
        };

        return std::make_unique<vulkan::VulkanInstance>(
                instance_extensions, device_extensions, required_device_features, optional_device_features,
                surface_function);
}
}
