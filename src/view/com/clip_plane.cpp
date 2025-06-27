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

#include <functional>
#include <optional>
#include <utility>
#include <variant>

namespace ns::view::com
{
namespace
{
numerical::Vector4d clip_plane_equation(const numerical::Vector4d& plane, const double position)
{
        ASSERT(position >= 0 && position <= 1);

        return {
                plane[0],
                plane[1],
                plane[2],
                plane[3] + 2 * position - 1,
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
        if (!plane_)
        {
                error("Clip plane is not set");
        }

        if (!(position >= 0 && position <= 1))
        {
                error("Error clip plane position " + to_string(position));
        }

        position_ = position;
        set_clip_plane_(clip_plane_equation(*plane_, position_));
}

void ClipPlane::cmd(const command::ClipPlaneHide&)
{
        plane_.reset();
        set_clip_plane_(std::nullopt);
}

void ClipPlane::cmd(const command::ClipPlaneSetPosition& v)
{
        set_position(v.position);
}

void ClipPlane::cmd(const command::ClipPlaneShow& v)
{
        plane_ = -camera_->camera_plane();
        ASSERT(numerical::Vector3d((*plane_)[0], (*plane_)[1], (*plane_)[2]).is_unit());
        set_position(v.position);
}

std::optional<numerical::Vector4d> ClipPlane::equation() const
{
        if (!plane_)
        {
                return std::nullopt;
        }
        return clip_plane_equation(*plane_, position_);
}

[[nodiscard]] std::optional<double> ClipPlane::position() const
{
        if (!plane_)
        {
                return std::nullopt;
        }
        return position_;
}
}
