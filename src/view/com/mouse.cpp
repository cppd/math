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

#include "mouse.h"

#include "camera.h"

#include <src/numerical/region.h>
#include <src/numerical/vector.h>
#include <src/view/event.h>

#include <cmath>
#include <tuple>
#include <variant>

namespace ns::view
{
Mouse::Mouse(Camera* const camera)
        : camera_(camera)
{
}

void Mouse::set_rectangle(const numerical::Region<2, int>& rectangle, const int width, const int height)
{
        rectangle_ = rectangle;
        width_ = width;
        height_ = height;
}

std::tuple<int, int> Mouse::position(const double x, const double y) const
{
        return {std::lround(x * width_), std::lround(y * height_)};
}

void Mouse::exec(const MouseCommand& command)
{
        std::visit(
                [this](const auto& v)
                {
                        cmd(v);
                },
                command);
}

const Mouse::MouseButtonInfo& Mouse::info(const MouseButton button) const
{
        const auto iter = buttons_.find(button);
        if (iter != buttons_.cend())
        {
                return iter->second;
        }
        static constexpr MouseButtonInfo INFO{};
        return INFO;
}

void Mouse::cmd(const command::MousePress& v)
{
        const auto [x, y] = position(v.x, v.y);

        x_ = x;
        y_ = y;

        MouseButtonInfo& m = buttons_[v.button];
        m.pressed = true;
        m.pressed_x = x;
        m.pressed_y = y;
        m.delta_x = 0;
        m.delta_y = 0;
}

void Mouse::cmd(const command::MouseRelease& v)
{
        const auto [x, y] = position(v.x, v.y);

        buttons_[v.button].pressed = false;
        x_ = x;
        y_ = y;
}

void Mouse::cmd(const command::MouseMove& v)
{
        const auto [x, y] = position(v.x, v.y);

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
                camera_->move(numerical::Vector2d(-left.delta_x, left.delta_y));
        }
}

void Mouse::cmd(const command::MouseWheel& v)
{
        const auto [x, y] = position(v.x, v.y);

        camera_->scale(x - rectangle_.x0(), y - rectangle_.y0(), v.delta);
}
}
