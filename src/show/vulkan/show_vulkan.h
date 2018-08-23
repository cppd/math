/*
Copyright (C) 2017, 2018 Topological Manifold

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

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)

#include "window/window_handle.h"

#include <memory>

class IShowVulkan
{
public:
        virtual ~IShowVulkan() = default;

        virtual void parent_resized() = 0;
        virtual void mouse_wheel(double) = 0;
};

std::unique_ptr<IShowVulkan> create_show_vulkan(WindowID win_parent);

#endif
