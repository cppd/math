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

#include "camera.h"

#include "../event.h"

#include <src/com/type/limit.h>
#include <src/numerical/region.h>

#include <unordered_map>

namespace ns::view
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
        Region<2, int> rectangle_{{Limits<int>::lowest(), Limits<int>::lowest()}, {0, 0}};

        const MouseButtonInfo& info(MouseButton button) const;

        void command(const command::MousePress& v);
        void command(const command::MouseRelease& v);
        void command(const command::MouseMove& v);
        void command(const command::MouseWheel& v);

public:
        explicit Mouse(Camera* camera);

        void set_rectangle(const Region<2, int>& rectangle);
        void command(const MouseCommand& mouse_command);
};
}
