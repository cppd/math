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

#pragma once

#include <SFML/Window/WindowHandle.hpp>

using WindowID = sf::WindowHandle;

void move_window_to_parent(WindowID window, WindowID parent);
void make_window_fullscreen(WindowID window);
void set_focus(WindowID window);
void set_size_to_parent(WindowID window, WindowID parent);

#if defined(_WIN32)
void change_window_style_not_child(WindowID window);
#endif
