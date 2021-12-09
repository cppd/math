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

#include "surface.h"

#include <src/vulkan/error.h>

#if defined(__linux__)

#include <QX11Info>
//
#include <vulkan/vulkan_xcb.h>

namespace ns::window
{
std::vector<std::string> vulkan_create_surface_required_extensions()
{
        return {"VK_KHR_surface", "VK_KHR_xcb_surface"};
}

VkSurfaceKHR vulkan_create_surface(const WindowID window, const VkInstance instance)
{
        VkXcbSurfaceCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        info.connection = QX11Info::connection();
        info.window = window;

        VkSurfaceKHR surface;
        VULKAN_CHECK(vkCreateXcbSurfaceKHR(instance, &info, nullptr, &surface));
        return surface;
}
}

#elif defined(_WIN32)

#include <windows.h>
//
#include <vulkan/vulkan_win32.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace ns::window
{
std::vector<std::string> vulkan_create_surface_required_extensions()
{
        return {"VK_KHR_surface", "VK_KHR_win32_surface"};
}

VkSurfaceKHR vulkan_create_surface(const WindowID window, const VkInstance instance)
{
        VkWin32SurfaceCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        info.hinstance = GetModuleHandle(NULL);
        info.hwnd = window;

        VkSurfaceKHR surface;
        VULKAN_CHECK(vkCreateWin32SurfaceKHR(instance, &info, nullptr, &surface));
        return surface;
}
}

#pragma GCC diagnostic pop

#else
#error This operating system is not supported
#endif
