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

#if 0

#include "handle.h"

#include <vulkan/vulkan.h>

#include <string>

namespace ns::window
{
std::string vulkan_create_surface_extension();
VkSurfaceKHR vulkan_create_surface(WindowID window, VkInstance instance);

class WindowInit final
{
public:
        WindowInit();
        ~WindowInit();

        WindowInit(const WindowInit&) = delete;
        WindowInit(WindowInit&&) = delete;
        WindowInit& operator=(const WindowInit&) = delete;
        WindowInit& operator=(WindowInit&&) = delete;
};
}

#endif
