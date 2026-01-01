/*
Copyright (C) 2017-2026 Topological Manifold

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

#if 0

#include "window/event.h"
#include "window/handle.h"

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace ns::window
{
void window_init();
void window_terminate();

class Window
{
public:
        // static std::vector<std::string> instance_extensions();

        //

        virtual ~Window() = default;

        virtual WindowID system_handle() = 0;
        virtual int width() const = 0;
        virtual int height() const = 0;
        virtual void pull_and_dispath_events(WindowEvent& window_event) = 0;

        // virtual VkSurfaceKHR create_surface(VkInstance instance) = 0;
};

std::unique_ptr<Window> create_window();
}

#endif
