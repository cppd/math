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

#include <memory>

namespace opengl
{
void window_init();

class Context
{
        class Impl;
        std::unique_ptr<Impl> m_impl;

public:
        Context();
        ~Context();
};

class Window
{
public:
        virtual ~Window() = default;

        virtual WindowID system_handle() const = 0;
        virtual int width() const = 0;
        virtual int height() const = 0;
        virtual void pull_and_dispath_events(WindowEvent& window_event) = 0;

        virtual void set_vertical_sync_enabled(bool v) = 0;
        virtual void display() = 0;
};

std::unique_ptr<Window> create_window(int minimum_sample_count);
}
