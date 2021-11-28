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

#include "clip_plane.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/gpu/renderer/renderer.h>

namespace ns::view
{
namespace
{
Vector4d create_clip_plane(const Matrix4d& clip_plane_view, const double position)
{
        ASSERT(position >= 0 && position <= 1);

        // -z = 0 or (0, 0, -1, 0).
        // (0, 0, -1, 0) * view matrix.
        Vector4d plane = -clip_plane_view.row(2);

        const Vector3d n(plane[0], plane[1], plane[2]);
        const double d = n.norm_1();

        // -z = d * (1 - 2 * position)
        // (0, 0, -1, d * (2 * position - 1)).
        plane[3] += d * (2 * position - 1);

        plane /= n.norm();

        return plane;
}
}

template <typename Renderer>
ClipPlane<Renderer>::ClipPlane(Renderer* const renderer, const Camera* const camera)
        : renderer_(renderer), camera_(camera)
{
}

template <typename Renderer>
void ClipPlane<Renderer>::command(const ClipPlaneCommand& clip_plane_command)
{
        const auto visitor = [&](const auto& v)
        {
                command(v);
        };
        std::visit(visitor, clip_plane_command);
}

template <typename Renderer>
void ClipPlane<Renderer>::set_position(const double position)
{
        if (!matrix_)
        {
                error("Clip plane is not set");
        }

        if (!(position >= 0 && position <= 1))
        {
                error("Error clip plane position " + to_string(position));
        }

        renderer_->set_clip_plane(create_clip_plane(*matrix_, position));
}

template <typename Renderer>
void ClipPlane<Renderer>::command(const command::ClipPlaneHide&)
{
        matrix_.reset();
        renderer_->set_clip_plane(std::nullopt);
}

template <typename Renderer>
void ClipPlane<Renderer>::command(const command::ClipPlanePosition& v)
{
        set_position(v.position);
}

template <typename Renderer>
void ClipPlane<Renderer>::command(const command::ClipPlaneShow& v)
{
        matrix_ = camera_->renderer_info().main_view_matrix;
        set_position(v.position);
}

template <typename Renderer>
void ClipPlane<Renderer>::command(const command::SetClipPlaneColor& v)
{
        renderer_->set_clip_plane_color(v.value);
}

template class ClipPlane<gpu::renderer::Renderer>;
}
