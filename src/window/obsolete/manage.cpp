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

#if 0

#include "manage.h"

#include <src/vulkan/error.h>

#if defined(__linux__)

#include <src/com/error.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <array>
#include <atomic>
#include <cstdlib>
//
#include <vulkan/vulkan_xlib.h>

namespace ns::window
{
namespace
{
int error_handler(Display*, XErrorEvent* const e)
{
        constexpr int BUF_SIZE = 1000;
        std::array<char, BUF_SIZE> buf;
        XGetErrorText(e->display, e->error_code, buf.data(), BUF_SIZE);
        error_fatal("X error handler: " + std::string(buf.data()));
}

Display* g_display = nullptr;

// constexpr Atom xa_atom()
// {
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wold-style-cast"
//         return XA_ATOM;
// #pragma GCC diagnostic pop
// }
}

WindowInit::WindowInit()
{
        static std::atomic_int call_counter = 0;
        if (++call_counter != 1)
        {
                error_fatal("WindowInit must be called once");
        }

        if (!XInitThreads())
        {
                error_fatal("Error XInitThreads");
        }

        XSetErrorHandler(error_handler);

        g_display = XOpenDisplay(nullptr); // getenv("DISPLAY")
        if (!g_display)
        {
                error("Error XOpenDiplay");
        }

        XSynchronize(g_display, False);
}

WindowInit::~WindowInit()
{
        if (g_display)
        {
                XCloseDisplay(g_display);
        }
        XSetErrorHandler(nullptr);
}

// void move_window_to_parent(const WindowID window, const WindowID parent)
// {
//         // https://standards.freedesktop.org/wm-spec/wm-spec-1.5.html
//
//         Display* const display = g_display;
//
//         XUnmapWindow(display, window);
//
// #if 1
//         const Atom wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
//         Atom wm_window_type_toolbar = XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
//         const unsigned char* const toolbar = reinterpret_cast<const unsigned char*>(&wm_window_type_toolbar);
//         XChangeProperty(display, window, wm_window_type, xa_atom(), 32, PropModeAppend, toolbar, 1);
// #else
//         const Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
//         Atom wm_state_skip_taskbar = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
//         Atom wm_state_skip_pager = XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", False);
//         const unsigned char* const skip_taskbar = reinterpret_cast<const unsigned char*>(&wm_state_skip_taskbar);
//         const unsigned char* const skip_pager = reinterpret_cast<const unsigned char*>(&wm_state_skip_pager);
//         XChangeProperty(display, window, wm_state, xa_atom(), 32, PropModeAppend, skip_taskbar, 1);
//         XChangeProperty(display, window, wm_state, xa_atom(), 32, PropModeAppend, skip_pager, 1);
// #endif
//
//         XReparentWindow(display, window, parent, 0, 0);
//
//         XWindowAttributes parent_attr;
//         XGetWindowAttributes(display, parent, &parent_attr);
//         XResizeWindow(display, window, parent_attr.width, parent_attr.height);
//
//         XMapWindow(display, window);
// }

// void make_window_fullscreen(const WindowID window)
// {
//         // https://standards.freedesktop.org/wm-spec/wm-spec-1.5.html
//
//         Display* const display = g_display;
//
//         XUnmapWindow(display, window);
//
//         const Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
//         Atom wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
//         const unsigned char* const fullscreen = reinterpret_cast<const unsigned char*>(&wm_state_fullscreen);
//         XChangeProperty(display, window, wm_state, xa_atom(), 32, PropModeAppend, fullscreen, 1);
//
//         XReparentWindow(display, window, XDefaultRootWindow(display), 0, 0);
//
//         XMapWindow(display, window);
// }

// void set_focus(WindowID window)
// {
//         Display* const display = g_display;
//
//         XSetInputFocus(display, window, RevertToParent, CurrentTime);
// }

// void set_size_to_parent(WindowID window, WindowID parent)
// {
//         Display* const display = g_display;
//
//         XWindowAttributes parent_attr;
//         XGetWindowAttributes(display, parent, &parent_attr);
//         XResizeWindow(display, window, parent_attr.width, parent_attr.height);
// }

std::string vulkan_create_surface_extension()
{
        return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

VkSurfaceKHR vulkan_create_surface(WindowID window, VkInstance instance)
{
        Display* const display = g_display;

        VkXlibSurfaceCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        info.dpy = display;
        info.window = window;

        VkSurfaceKHR surface;
        VULKAN_CHECK(vkCreateXlibSurfaceKHR(instance, &info, nullptr, &surface));
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
WindowInit::WindowInit()
{
}

WindowInit::~WindowInit()
{
}

// void move_window_to_parent(WindowID window, WindowID parent)
// {
//         SetWindowLongPtr(window, GWL_STYLE, WS_CHILD | WS_VISIBLE);
//
//         SetParent(window, parent);
//
//         RECT rect;
//         GetClientRect(parent, &rect);
//         SetWindowPos(window, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 0);
//
//         SetFocus(parent);
// }

// void change_window_style_not_child(WindowID window)
// {
//         SetWindowLongPtr(window, GWL_STYLE, WS_POPUP);
// }

// void make_window_fullscreen(WindowID window)
// {
//         SetWindowLongPtr(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
//
//         SetParent(window, nullptr);
//
//         SetWindowPos(window, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
// }

// void set_focus(WindowID window)
// {
//         SetFocus(window);
// }

// void set_size_to_parent(WindowID window, WindowID parent)
// {
//         RECT rect;
//         GetClientRect(parent, &rect);
//         SetWindowPos(window, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 0);
// }

std::string vulkan_create_surface_extension()
{
        return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

VkSurfaceKHR vulkan_create_surface(WindowID window, VkInstance instance)
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

#endif
