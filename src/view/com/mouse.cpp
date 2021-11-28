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

#include "mouse.h"

namespace ns::view
{
Mouse::Mouse(Camera* const camera) : camera_(camera)
{
}

void Mouse::set_rectangle(const Region<2, int>& rectangle)
{
        rectangle_ = rectangle;
}

void Mouse::command(const MouseCommand& mouse_command)
{
        const auto visitor = [this](const auto& v)
        {
                command(v);
        };
        std::visit(visitor, mouse_command);
}

const Mouse::MouseButtonInfo& Mouse::info(const MouseButton button) const
{
        const auto iter = buttons_.find(button);
        if (iter != buttons_.cend())
        {
                return iter->second;
        }
        static constexpr const MouseButtonInfo INFO{};
        return INFO;
}

void Mouse::command(const command::MousePress& v)
{
        x_ = v.x;
        y_ = v.y;

        MouseButtonInfo& m = buttons_[v.button];
        m.pressed = true;
        m.pressed_x = v.x;
        m.pressed_y = v.y;
        m.delta_x = 0;
        m.delta_y = 0;
}

void Mouse::command(const command::MouseRelease& v)
{
        buttons_[v.button].pressed = false;
        x_ = v.x;
        y_ = v.y;
}

void Mouse::command(const command::MouseMove& v)
{
        for (auto& [button, info] : buttons_)
        {
                if (info.pressed)
                {
                        info.delta_x = v.x - x_;
                        info.delta_y = v.y - y_;
                }
        }

        x_ = v.x;
        y_ = v.y;

        const MouseButtonInfo& right = info(MouseButton::RIGHT);
        if (right.pressed && rectangle_.is_inside(right.pressed_x, right.pressed_y)
            && (right.delta_x != 0 || right.delta_y != 0))
        {
                camera_->rotate(-right.delta_x, -right.delta_y);
        }

        const MouseButtonInfo& left = info(MouseButton::LEFT);
        if (left.pressed && rectangle_.is_inside(left.pressed_x, left.pressed_y)
            && (left.delta_x != 0 || left.delta_y != 0))
        {
                camera_->move(Vector2d(-left.delta_x, left.delta_y));
        }
}

void Mouse::command(const command::MouseWheel& v)
{
        camera_->scale(v.x - rectangle_.x0(), v.y - rectangle_.y0(), v.delta);
}
}
