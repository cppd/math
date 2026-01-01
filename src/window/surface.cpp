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

#include "surface.h"

#include "handle.h"

#include <src/vulkan/error.h>
#include <src/vulkan/extensions.h>

#include <vulkan/vulkan_core.h>

#include <string>

#ifdef __linux__

#include <QGuiApplication>

#include <xcb/xcb.h>
//
#include <vulkan/vulkan_xcb.h>

namespace ns::window
{
std::string vulkan_create_surface_extension()
{
        return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
}

VkSurfaceKHR vulkan_create_surface(const WindowID window, const VkInstance instance)
{
        const auto create_surface = VULKAN_INSTANCE_PROC_ADDR(instance, vkCreateXcbSurfaceKHR);

        VkXcbSurfaceCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        info.connection = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
        info.window = window;

        VkSurfaceKHR surface;
        VULKAN_CHECK(create_surface(instance, &info, nullptr, &surface));
        return surface;
}
}

#elifdef _WIN32

#include <windows.h>
//
#include <vulkan/vulkan_win32.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace ns::window
{
std::string vulkan_create_surface_extension()
{
        return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

VkSurfaceKHR vulkan_create_surface(const WindowID window, const VkInstance instance)
{
        const auto create_surface = VULKAN_INSTANCE_PROC_ADDR(instance, vkCreateWin32SurfaceKHR);

        VkWin32SurfaceCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        info.hinstance = GetModuleHandle(NULL);
        info.hwnd = window;

        VkSurfaceKHR surface;
        VULKAN_CHECK(create_surface(instance, &info, nullptr, &surface));
        return surface;
}
}

#pragma GCC diagnostic pop

#else
#error This operating system is not supported
#endif
