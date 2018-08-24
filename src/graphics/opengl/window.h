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

#include "graphics/window_event.h"
#include "window/window_handle.h"

#include <memory>

class OpenGLContext
{
        class Impl;
        std::unique_ptr<Impl> m_impl;

public:
        OpenGLContext();
        ~OpenGLContext();
};

class OpenGLWindow
{
public:
        virtual ~OpenGLWindow() = default;

        virtual WindowID get_system_handle() const = 0;
        virtual int get_width() const = 0;
        virtual int get_height() const = 0;
        virtual void pull_and_dispath_events() = 0;

        virtual void set_vertical_sync_enabled(bool v) = 0;
        virtual void display() = 0;
};

std::unique_ptr<OpenGLWindow> create_opengl_window(WindowEvent* event_interface);
