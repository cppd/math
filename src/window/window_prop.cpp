/*
Copyright (C) 2017 Topological Manifold

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

#include "window_prop.h"

#include "com/error.h"

#if defined(__linux__)

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <cstdlib>

namespace
{
class CDisplay
{
        Display* m_display;

public:
        CDisplay()
        {
                m_display = XOpenDisplay(nullptr); // getenv( "DISPLAY" ) );
                if (m_display == nullptr)
                {
                        error("Error XOpenDiplay");
                }
                XSynchronize(m_display, False);
        }
        ~CDisplay()
        {
                XCloseDisplay(m_display);
        }
        Display* get_display() noexcept
        {
                return m_display;
        }
};

constexpr Atom get_xa_atom()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        return XA_ATOM;
#pragma GCC diagnostic pop
}
}

void move_window_to_parent(WindowID window, WindowID parent)
{
        // https://standards.freedesktop.org/wm-spec/wm-spec-1.5.html

        CDisplay d;
        Display* display = d.get_display();

        // В SFML окно уже показано, но свойства изменяются только перед mapping,
        // поэтому убрать окно, изменить свойства, показать окно.
        XUnmapWindow(display, window);

#if 1
        Atom wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
        Atom wm_window_type_toolbar = XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
        const unsigned char* toolbar = reinterpret_cast<const unsigned char*>(&wm_window_type_toolbar);
        XChangeProperty(display, window, wm_window_type, get_xa_atom(), 32, PropModeAppend, toolbar, 1);
#else
        Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
        Atom wm_state_skip_taskbar = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
        Atom wm_state_skip_pager = XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", False);
        const unsigned char* skip_taskbar = reinterpret_cast<const unsigned char*>(&wm_state_skip_taskbar);
        const unsigned char* skip_pager = reinterpret_cast<const unsigned char*>(&wm_state_skip_pager);
        XChangeProperty(display, window, wm_state, get_xa_atom(), 32, PropModeAppend, skip_taskbar, 1);
        XChangeProperty(display, window, wm_state, get_xa_atom(), 32, PropModeAppend, skip_pager, 1);
#endif

        // после изменения, но до отображения, сменить родительское окно
        XReparentWindow(display, window, parent, 0, 0);

        XWindowAttributes parent_attr;
        XGetWindowAttributes(display, parent, &parent_attr);
        XResizeWindow(display, window, parent_attr.width, parent_attr.height);

        // показать обратно
        XMapWindow(display, window);
}

void make_window_fullscreen(WindowID window)
{
        // https://standards.freedesktop.org/wm-spec/wm-spec-1.5.html

        CDisplay d;
        Display* display = d.get_display();

        // В SFML окно уже показано, но свойства изменяются только перед mapping,
        // поэтому убрать окно, изменить свойства, показать окно.
        XUnmapWindow(display, window);

        Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
        Atom wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
        const unsigned char* fullscreen = reinterpret_cast<const unsigned char*>(&wm_state_fullscreen);
        XChangeProperty(display, window, wm_state, get_xa_atom(), 32, PropModeAppend, fullscreen, 1);

        // после изменения, но до отображения, сменить родительское окно
        XReparentWindow(display, window, XDefaultRootWindow(display), 0, 0);

        // показать обратно
        XMapWindow(display, window);
}

void set_focus(WindowID window)
{
        CDisplay d;
        Display* display = d.get_display();

        XSetInputFocus(display, window, RevertToParent, CurrentTime);
}

void set_size_to_parent(WindowID window, WindowID parent)
{
        CDisplay d;
        Display* display = d.get_display();

        XWindowAttributes parent_attr;
        XGetWindowAttributes(display, parent, &parent_attr);
        XResizeWindow(display, window, parent_attr.width, parent_attr.height);
}

#else
#error This operating system is not supported
#endif
