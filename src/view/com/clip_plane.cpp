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

#include "clip_plane.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/gpu/renderer/renderer.h>

namespace ns::view
{
namespace
{
Vector4d clip_plane_equation(const Matrix4d& view_matrix, const double position)
{
        ASSERT(position >= 0 && position <= 1);

        // -z = 0 or (0, 0, -1, 0).
        // (0, 0, -1, 0) * view matrix.
        Vector4d plane = -view_matrix.row(2);

        const Vector3d n(plane[0], plane[1], plane[2]);
        const double d = n.norm_1();

        // -z = d * (1 - 2 * position)
        // (0, 0, -1, d * (2 * position - 1)).
        plane[3] += d * (2 * position - 1);

        plane /= n.norm();

        return plane;
}
}

ClipPlane::ClipPlane(const Camera* const camera, std::function<void(const std::optional<Vector4d>&)> set_clip_plane)
        : camera_(camera),
          set_clip_plane_(std::move(set_clip_plane))
{
}

void ClipPlane::command(const ClipPlaneCommand& clip_plane_command)
{
        const auto visitor = [this](const auto& v)
        {
                command(v);
        };
        std::visit(visitor, clip_plane_command);
}

void ClipPlane::set_position(const double position)
{
        if (!view_matrix_)
        {
                error("Clip plane is not set");
        }

        if (!(position >= 0 && position <= 1))
        {
                error("Error clip plane position " + to_string(position));
        }

        position_ = position;
        set_clip_plane_(clip_plane_equation(*view_matrix_, position_));
}

void ClipPlane::command(const command::ClipPlaneHide&)
{
        view_matrix_.reset();
        set_clip_plane_(std::nullopt);
}

void ClipPlane::command(const command::ClipPlaneSetPosition& v)
{
        set_position(v.position);
}

void ClipPlane::command(const command::ClipPlaneShow& v)
{
        view_matrix_ = camera_->view_matrix();
        set_position(v.position);
}

std::optional<Vector4d> ClipPlane::equation() const
{
        if (!view_matrix_)
        {
                return std::nullopt;
        }
        return clip_plane_equation(*view_matrix_, position_);
}

[[nodiscard]] std::optional<double> ClipPlane::position() const
{
        if (!view_matrix_)
        {
                return std::nullopt;
        }
        return position_;
}
}
