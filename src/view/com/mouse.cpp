/*
Copyright (C) 2017-2026 Topological Manifold

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

namespace ns::view::com
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

void Mouse::cmd(const command::MousePress& v)
{
        const auto [x, y] = position(v.x, v.y);

        x_ = x;
        y_ = y;

        pressed_[v.button] = rectangle_.is_inside(x, y);
}

void Mouse::cmd(const command::MouseRelease& v)
{
        const auto [x, y] = position(v.x, v.y);

        x_ = x;
        y_ = y;

        pressed_[v.button] = false;
}

void Mouse::cmd(const command::MouseMove& v)
{
        const auto [x, y] = position(v.x, v.y);

        if (pressed_[MouseButton::RIGHT] && (x != x_ || y != y_))
        {
                camera_->rotate(x_ - x, y_ - y);
        }

        if (pressed_[MouseButton::LEFT] && (x != x_ || y != y_))
        {
                camera_->move({x_ - x, y - y_});
        }

        x_ = x;
        y_ = y;
}

void Mouse::cmd(const command::MouseWheel& v)
{
        const auto [x, y] = position(v.x, v.y);

        camera_->scale(x - rectangle_.x0(), y - rectangle_.y0(), v.delta);
}
}
