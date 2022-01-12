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

#include "instance.h"

#include <src/window/surface.h>

namespace ns::view
{
std::unique_ptr<vulkan::VulkanInstance> create_instance(
        const window::WindowID window,
        std::vector<std::string> required_device_extensions,
        const std::vector<std::string>& optional_device_extensions,
        const vulkan::PhysicalDeviceFeatures& required_device_features,
        const vulkan::PhysicalDeviceFeatures& optional_device_features)
{
        const std::vector<std::string> required_instance_extensions =
                window::vulkan_create_surface_required_extensions();

        required_device_extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        const std::function<VkSurfaceKHR(VkInstance)> create_surface = [&](const VkInstance instance)
        {
                return window::vulkan_create_surface(window, instance);
        };

        return std::make_unique<vulkan::VulkanInstance>(
                required_instance_extensions, required_device_extensions, optional_device_extensions,
                required_device_features, optional_device_features, create_surface);
}
}
