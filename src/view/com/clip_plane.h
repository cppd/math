/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/color/color.h>
#include <src/numerical/matrix.h>

#include <functional>
#include <optional>

namespace ns::view
{
class ClipPlane final
{
        const Camera* camera_;
        std::optional<Matrix4d> view_matrix_;
        double position_;
        std::function<void(const std::optional<Vector4d>&)> set_clip_plane_;

        void set_position(double position);

        void command(const command::ClipPlaneHide&);
        void command(const command::ClipPlaneSetPosition& v);
        void command(const command::ClipPlaneShow& v);

public:
        ClipPlane(const Camera* camera, std::function<void(const std::optional<Vector4d>&)> set_clip_plane);

        void command(const ClipPlaneCommand& clip_plane_command);

        [[nodiscard]] std::optional<Vector4d> equation() const;
        [[nodiscard]] std::optional<double> position() const;
};
}
