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

#pragma once

#include "../event.h"

#include <src/com/type/limit.h>

#include <unordered_map>

namespace ns::view
{
struct MouseButtonInfo final
{
        bool pressed = false;
        int pressed_x;
        int pressed_y;
        int delta_x;
        int delta_y;
};

class Mouse final
{
        std::unordered_map<MouseButton, MouseButtonInfo> buttons_;
        int x_ = limits<int>::lowest();
        int y_ = limits<int>::lowest();

public:
        const MouseButtonInfo& info(const MouseButton button) const
        {
                auto iter = buttons_.find(button);
                if (iter != buttons_.cend())
                {
                        return iter->second;
                }

                thread_local const MouseButtonInfo info{};
                return info;
        }

        void press(const int x, const int y, const MouseButton button)
        {
                x_ = x;
                y_ = y;
                MouseButtonInfo& m = buttons_[button];
                m.pressed = true;
                m.pressed_x = x;
                m.pressed_y = y;
                m.delta_x = 0;
                m.delta_y = 0;
        }

        void release(const int x, const int y, const MouseButton button)
        {
                buttons_[button].pressed = false;
                x_ = x;
                y_ = y;
        }

        void move(const int x, const int y)
        {
                for (auto& [button, info] : buttons_)
                {
                        if (info.pressed)
                        {
                                info.delta_x = x - x_;
                                info.delta_y = y - y_;
                        }
                }
                x_ = x;
                y_ = y;
        }
};
}
