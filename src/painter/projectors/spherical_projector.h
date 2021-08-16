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

#include "functions.h"

#include "../objects.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

namespace ns::painter
{
// Параллельное проецирование точек экрана на полусферу и создание лучей
// из центра полусферы в направлении точек на сфере.
template <std::size_t N, typename T>
class SphericalProjector final : public Projector<N, T>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        std::array<int, N - 1> screen_size_;
        std::array<Vector<N, T>, N - 1> screen_axes_;
        Vector<N - 1, T> screen_org_;
        Vector<N, T> camera_org_;
        Vector<N, T> camera_dir_;

        T square_radius_;

public:
        SphericalProjector(
                const Vector<N, T>& camera_org,
                const Vector<N, T>& camera_dir,
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                T width_view_angle_degrees,
                const std::array<int, N - 1>& screen_size)
        {
                screen_size_ = screen_size;
                screen_org_ = projectors_implementation::screen_org<T>(screen_size);
                camera_org_ = camera_org;
                camera_dir_ = camera_dir.normalized();
                screen_axes_ = projectors_implementation::normalize_axes(screen_axes);

                projectors_implementation::check_orthogonality(camera_dir_, screen_axes_);

                //

                T half_angle = width_view_angle_degrees * T(0.5) * PI<T> / 180;
                T sin_alpha = std::sin(half_angle);

                if (!(width_view_angle_degrees > 0))
                {
                        error("Spherical projection view angle " + to_string(width_view_angle_degrees)
                              + " is not positive");
                }
                T k = sin_alpha / screen_size[0];
                T r = square(sin_alpha);
                for (unsigned i = 1; i < N - 1; ++i)
                {
                        r += square(k * screen_size[i]);
                }
                if (!(r < 1))
                {
                        error("Spherical projection view angle " + to_string(width_view_angle_degrees) + " is too big");
                }

                square_radius_ = square(screen_size[0] * T(0.5) / sin_alpha);
        }

        const std::array<int, N - 1>& screen_size() const override
        {
                return screen_size_;
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const override
        {
                Vector<N - 1, T> screen_point = screen_org_ + point;

                T radicand = square_radius_ - dot(screen_point, screen_point);
                if (!(radicand > 0))
                {
                        error("Error spherical projection point " + to_string(point));
                }
                T z = std::sqrt(radicand);

                Vector<N, T> screen_dir = projectors_implementation::screen_dir(screen_axes_, screen_point);
                return Ray<N, T>(camera_org_, camera_dir_ * z + screen_dir);
        }
};
}
