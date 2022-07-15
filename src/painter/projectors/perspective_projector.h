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

#include "functions.h"

#include "../objects.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class PerspectiveProjector final : public Projector<N, T>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        static Vector<N, T> make_camera_dir(
                const Vector<N, T>& camera_dir,
                const T width_view_angle_degrees,
                const std::array<int, N - 1>& screen_size)
        {
                if (!(width_view_angle_degrees > 0 && width_view_angle_degrees < 180))
                {
                        error("Perspective projection: error view angle " + to_string(width_view_angle_degrees));
                }

                const T half_angle = width_view_angle_degrees * T(0.5) * PI<T> / 180;
                const T dir_length = screen_size[0] * T(0.5) * std::tan(PI<T> / 2 - half_angle);

                return camera_dir.normalized() * dir_length;
        }

        std::array<int, N - 1> screen_size_;
        std::array<Vector<N, T>, N - 1> screen_axes_;
        Vector<N - 1, T> screen_org_;
        Vector<N, T> camera_org_;
        Vector<N, T> camera_dir_;

public:
        PerspectiveProjector(
                const Vector<N, T>& camera_org,
                const Vector<N, T>& camera_dir,
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                const T width_view_angle_degrees,
                const std::array<int, N - 1>& screen_size)
                : screen_size_(screen_size),
                  screen_axes_(projectors_implementation::normalize_axes(screen_axes)),
                  screen_org_(projectors_implementation::screen_org<T>(screen_size)),
                  camera_org_(camera_org),
                  camera_dir_(make_camera_dir(camera_dir, width_view_angle_degrees, screen_size))
        {
                projectors_implementation::check_orthogonality(camera_dir_, screen_axes_);
        }

        [[nodiscard]] const std::array<int, N - 1>& screen_size() const override
        {
                return screen_size_;
        }

        [[nodiscard]] Ray<N, T> ray(const Vector<N - 1, T>& point) const override
        {
                const Vector<N - 1, T> screen_point = screen_org_ + point;
                const Vector<N, T> screen_dir = projectors_implementation::screen_dir(screen_axes_, screen_point);
                return Ray<N, T>(camera_org_, camera_dir_ + screen_dir);
        }
};
}
