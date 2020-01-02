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

#pragma once

#include "com/error.h"
#include "com/print.h"
#include "window/event.h"

#include <unordered_map>

struct PressedMouseButton
{
        bool pressed = false;
        int pressed_x;
        int pressed_y;
        int delta_x;
        int delta_y;
};

template <typename Window>
class EventWindow final : public WindowEvent
{
        static constexpr int EMPTY = std::numeric_limits<int>::lowest();

        Window* m_window = nullptr;
        WindowEvent* m_window_event = nullptr;

        std::unordered_map<MouseButton, PressedMouseButton> m_mouse;
        int m_mouse_x = EMPTY;
        int m_mouse_y = EMPTY;
        int m_window_width = EMPTY;
        int m_window_height = EMPTY;

        void window_keyboard_pressed(KeyboardButton button) override
        {
                m_window_event->window_keyboard_pressed(button);
        }

        void window_mouse_pressed(MouseButton button) override
        {
                PressedMouseButton& m = m_mouse[button];

                m.pressed = true;
                m.pressed_x = m_mouse_x;
                m.pressed_y = m_mouse_y;
                m.delta_x = 0;
                m.delta_y = 0;

                m_window_event->window_mouse_pressed(button);
        }

        void window_mouse_released(MouseButton button) override
        {
                m_mouse[button].pressed = false;

                m_window_event->window_mouse_released(button);
        }

        void window_mouse_moved(int x, int y) override
        {
                for (auto& [button, m] : m_mouse)
                {
                        if (m.pressed)
                        {
                                m.delta_x = x - m_mouse_x;
                                m.delta_y = y - m_mouse_y;
                        }
                }
                m_mouse_x = x;
                m_mouse_y = y;

                m_window_event->window_mouse_moved(x, y);
        }

        void window_mouse_wheel(int delta) override
        {
                m_window_event->window_mouse_wheel(delta);
        }

        void window_resized(int width, int height) override
        {
                if (width <= 0 || height <= 0)
                {
                        error("Window resize error: width = " + to_string(width) + ", height = " + to_string(height));
                }

                m_window_width = width;
                m_window_height = height;

                m_window_event->window_resized(width, height);
        }

public:
        void set_window(Window& window)
        {
                m_window = &window;
        }

        void pull_and_dispath_events(WindowEvent& window_event)
        {
                m_window_event = &window_event;
                m_window->pull_and_dispath_events(*this);
        }

        const PressedMouseButton& pressed_mouse_button(MouseButton button) const
        {
                auto iter = m_mouse.find(button);
                if (iter != m_mouse.cend())
                {
                        return iter->second;
                }
                else
                {
                        thread_local const PressedMouseButton m{};
                        return m;
                }
        }

        int mouse_x() const
        {
                return m_mouse_x;
        }

        int mouse_y() const
        {
                return m_mouse_y;
        }

        int window_width() const
        {
                return m_window_width;
        }

        int window_height() const
        {
                return m_window_height;
        }
};
