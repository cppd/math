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

#include "clip_plane.h"

#include "camera.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>
#include <src/view/event.h>

#include <cmath>
#include <functional>
#include <optional>
#include <utility>
#include <variant>

namespace ns::view::com
{
namespace
{
template <typename T>
numerical::Vector4d clip_plane_equation(const T& camera_plane, const double position)
{
        ASSERT(camera_plane.normal.is_unit());
        ASSERT(camera_plane.near > camera_plane.far);
        ASSERT(position >= 0 && position <= 1);

        // camera plane: n * x = d
        // clip plane: -n * x = -d
        // -n * x + d = 0
        return {
                -camera_plane.normal[0],
                -camera_plane.normal[1],
                -camera_plane.normal[2],
                std::lerp(camera_plane.far, camera_plane.near, position),
        };
}
}

ClipPlane::ClipPlane(
        const Camera* const camera,
        std::function<void(const std::optional<numerical::Vector4d>&)> set_clip_plane)
        : camera_(camera),
          set_clip_plane_(std::move(set_clip_plane))
{
}

void ClipPlane::exec(const ClipPlaneCommand& command)
{
        std::visit(
                [this](const auto& v)
                {
                        cmd(v);
                },
                command);
}

void ClipPlane::set_position(const double position)
{
        if (!camera_plane_)
        {
                error("Clip plane is not set");
        }

        if (!(position >= 0 && position <= 1))
        {
                error("Error clip plane position " + to_string(position));
        }

        position_ = position;
        set_clip_plane_(clip_plane_equation(*camera_plane_, position_));
}

void ClipPlane::cmd(const command::ClipPlaneHide&)
{
        camera_plane_.reset();
        set_clip_plane_(std::nullopt);
}

void ClipPlane::cmd(const command::ClipPlaneSetPosition& v)
{
        set_position(v.position);
}

void ClipPlane::cmd(const command::ClipPlaneShow& v)
{
        camera_plane_ = camera_->plane();
        set_position(v.position);
}

std::optional<numerical::Vector4d> ClipPlane::equation() const
{
        if (!camera_plane_)
        {
                return std::nullopt;
        }
        return clip_plane_equation(*camera_plane_, position_);
}

[[nodiscard]] std::optional<double> ClipPlane::position() const
{
        if (!camera_plane_)
        {
                return std::nullopt;
        }
        return position_;
}
}
