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
#include <src/numerical/matrix.h>
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
numerical::Vector4d clip_plane_equation(const numerical::Vector4d& camera_plane, const double position)
{
        ASSERT(position >= 0 && position <= 1);

        numerical::Vector4d plane = -camera_plane;

        const numerical::Vector3d n(plane[0], plane[1], plane[2]);
        const double d = n.norm_1();

        // -z = d * (1 - 2 * position)
        // (0, 0, -1, d * (2 * position - 1)).
        plane[3] += d * (2 * position - 1);

        plane /= n.norm();

        return plane;
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
        camera_plane_ = camera_->camera_plane();
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
