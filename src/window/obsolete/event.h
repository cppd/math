/*
Copyright (C) 2017-2024 Topological Manifold

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

namespace ns::window
{
class WindowEvent
{
protected:
        virtual ~WindowEvent() = default;

public:
        enum class KeyboardButton
        {
                F11,
                Escape
        };

        enum class MouseButton
        {
                Left,
                Right
        };

        virtual void window_keyboard_pressed(KeyboardButton button) = 0;
        virtual void window_mouse_pressed(MouseButton button) = 0;
        virtual void window_mouse_released(MouseButton button) = 0;
        virtual void window_mouse_moved(int x, int y) = 0;
        virtual void window_mouse_wheel(int delta) = 0;
        virtual void window_resized(int width, int height) = 0;
};
}

#endif
