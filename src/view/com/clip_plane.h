/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/numerical/vector.h>
#include <src/view/event.h>

#include <functional>
#include <optional>

namespace ns::view::com
{
class ClipPlane final
{
        const Camera* camera_;
        std::optional<Camera::Plane> camera_plane_;
        double position_;
        std::function<void(const std::optional<numerical::Vector4d>&)> set_clip_plane_;

        void set_position(double position);

        void cmd(const command::ClipPlaneHide&);
        void cmd(const command::ClipPlaneSetPosition& v);
        void cmd(const command::ClipPlaneShow& v);

public:
        ClipPlane(const Camera* camera, std::function<void(const std::optional<numerical::Vector4d>&)> set_clip_plane);

        void exec(const ClipPlaneCommand& command);

        [[nodiscard]] std::optional<numerical::Vector4d> equation() const;
        [[nodiscard]] std::optional<double> position() const;
};
}
