/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "window/event.h"
#include "window/handle.h"

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

void vulkan_window_init();
void vulkan_window_terminate() noexcept;

class VulkanWindow
{
public:
        static std::vector<std::string> instance_extensions();

        //

        virtual ~VulkanWindow() = default;

        virtual WindowID system_handle() = 0;
        virtual int width() const = 0;
        virtual int height() const = 0;
        virtual void pull_and_dispath_events() = 0;

        virtual VkSurfaceKHR create_surface(VkInstance instance) = 0;
};

std::unique_ptr<VulkanWindow> create_vulkan_window(WindowEvent* event_interface);
