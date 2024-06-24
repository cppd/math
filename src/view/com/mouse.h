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

#include "camera.h"

#include <src/com/type/limit.h>
#include <src/numerical/region.h>
#include <src/view/event.h>

#include <tuple>
#include <unordered_map>

namespace ns::view::com
{
class Mouse final
{
        struct MouseButtonInfo final
        {
                bool pressed = false;
                int pressed_x;
                int pressed_y;
                int delta_x;
                int delta_y;
        };

        std::unordered_map<MouseButton, MouseButtonInfo> buttons_;

        int x_ = Limits<int>::lowest();
        int y_ = Limits<int>::lowest();

        Camera* camera_;
        numerical::Region<2, int> rectangle_{
                {Limits<int>::lowest(), Limits<int>::lowest()},
                {                    0,                     0}
        };

        double width_ = -1;
        double height_ = -1;

        [[nodiscard]] std::tuple<int, int> position(double x, double y) const;

        [[nodiscard]] const MouseButtonInfo& info(MouseButton button) const;

        void cmd(const command::MousePress& v);
        void cmd(const command::MouseRelease& v);
        void cmd(const command::MouseMove& v);
        void cmd(const command::MouseWheel& v);

public:
        explicit Mouse(Camera* camera);

        void set_rectangle(const numerical::Region<2, int>& rectangle, int width, int height);
        void exec(const MouseCommand& command);
};
}
