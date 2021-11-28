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

#include <src/numerical/matrix.h>

#include <optional>

namespace ns::view
{
template <typename Renderer>
class ClipPlane final
{
        std::optional<Matrix4d> matrix_;
        Renderer* renderer_;
        const Camera* camera_;

        void set_position(double position);

        void command(const command::ClipPlaneHide&);
        void command(const command::ClipPlaneSetPosition& v);
        void command(const command::ClipPlaneShow& v);
        void command(const command::ClipPlaneSetColor& v);

public:
        ClipPlane(Renderer* renderer, const Camera* camera);

        void command(const ClipPlaneCommand& clip_plane_command);
};
}
