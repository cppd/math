/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "manage.h"

#include <src/graphics/vulkan/error.h>

#if defined(__linux__)

#include <src/com/error.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <array>
#include <cstdlib>
//
#include <vulkan/vulkan_xlib.h>

namespace
{
int error_handler(Display*, XErrorEvent* e)
{
        constexpr int BUF_SIZE = 1000;
        std::array<char, BUF_SIZE> buf;
        XGetErrorText(e->display, e->error_code, buf.data(), BUF_SIZE);
        error_fatal("X error handler: " + std::string(buf.data()));
}

Display* global_display = nullptr;

// constexpr Atom xa_atom()
//{
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wold-style-cast"
//        return XA_ATOM;
//#pragma GCC diagnostic pop
//}
}

void xlib_init()
{
        if (!XInitThreads())
        {
                error_fatal("Error XInitThreads");
        }

        XSetErrorHandler(error_handler);

        global_display = XOpenDisplay(nullptr); // getenv("DISPLAY")
        if (global_display == nullptr)
        {
                error("Error XOpenDiplay");
        }

        XSynchronize(global_display, False);
}

void xlib_exit()
{
        if (global_display)
        {
                XCloseDisplay(global_display);
        }
        XSetErrorHandler(nullptr);
}

// void move_window_to_parent(WindowID window, WindowID parent)
//{
//        // https://standards.freedesktop.org/wm-spec/wm-spec-1.5.html

//        Display* display = global_display;

//        // В SFML окно уже показано, но свойства изменяются только перед mapping,
//        // поэтому убрать окно, изменить свойства, показать окно.
//        XUnmapWindow(display, window);

//#if 1
//        Atom wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
//        Atom wm_window_type_toolbar = XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
//        const unsigned char* toolbar = reinterpret_cast<const unsigned char*>(&wm_window_type_toolbar);
//        XChangeProperty(display, window, wm_window_type, xa_atom(), 32, PropModeAppend, toolbar, 1);
//#else
//        Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
//        Atom wm_state_skip_taskbar = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
//        Atom wm_state_skip_pager = XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", False);
//        const unsigned char* skip_taskbar = reinterpret_cast<const unsigned char*>(&wm_state_skip_taskbar);
//        const unsigned char* skip_pager = reinterpret_cast<const unsigned char*>(&wm_state_skip_pager);
//        XChangeProperty(display, window, wm_state, xa_atom(), 32, PropModeAppend, skip_taskbar, 1);
//        XChangeProperty(display, window, wm_state, xa_atom(), 32, PropModeAppend, skip_pager, 1);
//#endif

//        // после изменения, но до отображения, сменить родительское окно
//        XReparentWindow(display, window, parent, 0, 0);

//        XWindowAttributes parent_attr;
//        XGetWindowAttributes(display, parent, &parent_attr);
//        XResizeWindow(display, window, parent_attr.width, parent_attr.height);

//        // показать обратно
//        XMapWindow(display, window);
//}

// void make_window_fullscreen(WindowID window)
//{
//        // https://standards.freedesktop.org/wm-spec/wm-spec-1.5.html

//        Display* display = global_display;

//        // В SFML окно уже показано, но свойства изменяются только перед mapping,
//        // поэтому убрать окно, изменить свойства, показать окно.
//        XUnmapWindow(display, window);

//        Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
//        Atom wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
//        const unsigned char* fullscreen = reinterpret_cast<const unsigned char*>(&wm_state_fullscreen);
//        XChangeProperty(display, window, wm_state, xa_atom(), 32, PropModeAppend, fullscreen, 1);

//        // после изменения, но до отображения, сменить родительское окно
//        XReparentWindow(display, window, XDefaultRootWindow(display), 0, 0);

//        // показать обратно
//        XMapWindow(display, window);
//}

// void set_focus(WindowID window)
//{
//        Display* display = global_display;

//        XSetInputFocus(display, window, RevertToParent, CurrentTime);
//}

// void set_size_to_parent(WindowID window, WindowID parent)
//{
//        Display* display = global_display;

//        XWindowAttributes parent_attr;
//        XGetWindowAttributes(display, parent, &parent_attr);
//        XResizeWindow(display, window, parent_attr.width, parent_attr.height);
//}

std::vector<std::string> vulkan_create_surface_extensions()
{
        return {"VK_KHR_surface", "VK_KHR_xlib_surface"};
}

VkSurfaceKHR vulkan_create_surface(WindowID window, VkInstance instance)
{
        Display* display = global_display;

        VkXlibSurfaceCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        info.dpy = display;
        info.window = window;

        VkSurfaceKHR surface;
        VkResult result = vkCreateXlibSurfaceKHR(instance, &info, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateXlibSurfaceKHR", result);
        }
        return surface;
}

#elif defined(_WIN32)

#include <windows.h>
//
#include <vulkan/vulkan_win32.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

// void move_window_to_parent(WindowID window, WindowID parent)
//{
//        SetWindowLongPtr(window, GWL_STYLE, WS_CHILD | WS_VISIBLE);

//        SetParent(window, parent);

//        RECT rect;
//        GetClientRect(parent, &rect);
//        SetWindowPos(window, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 0);

//        SetFocus(parent);
//}

// void change_window_style_not_child(WindowID window)
//{
//        SetWindowLongPtr(window, GWL_STYLE, WS_POPUP);
//}

// void make_window_fullscreen(WindowID window)
//{
//        SetWindowLongPtr(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);

//        SetParent(window, nullptr);

//        SetWindowPos(window, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
//}

// void set_focus(WindowID window)
//{
//        SetFocus(window);
//}

// void set_size_to_parent(WindowID window, WindowID parent)
//{
//        RECT rect;
//        GetClientRect(parent, &rect);
//        SetWindowPos(window, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 0);
//}

std::vector<std::string> vulkan_create_surface_extensions()
{
        return {"VK_KHR_surface", "VK_KHR_win32_surface"};
}

VkSurfaceKHR vulkan_create_surface(WindowID window, VkInstance instance)
{
        VkWin32SurfaceCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        info.hinstance = GetModuleHandle(NULL);
        info.hwnd = window;

        VkSurfaceKHR surface;
        VkResult result = vkCreateWin32SurfaceKHR(instance, &info, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateWin32SurfaceKHR", result);
        }
        return surface;
}

#pragma GCC diagnostic pop

#else
#error This operating system is not supported
#endif
