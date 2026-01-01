/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "spherical_projector.h"

#include "com/functions.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace ns::painter::projectors
{
namespace
{
template <typename T, std::size_t N>
T make_square_radius(const std::type_identity_t<T> width_view_angle_degrees, const std::array<int, N>& screen_size)
{
        if (!(width_view_angle_degrees > 0))
        {
                error("Spherical projection view angle " + to_string(width_view_angle_degrees) + " is not positive");
        }

        const T half_angle = degrees_to_radians(width_view_angle_degrees * T{0.5});
        const T sin_alpha = std::sin(half_angle);

        const T k = sin_alpha / screen_size[0];
        T r = square(sin_alpha);
        for (std::size_t i = 1; i < N; ++i)
        {
                r += square(k * screen_size[i]);
        }
        if (!(r < 1))
        {
                error("Spherical projection view angle " + to_string(width_view_angle_degrees) + " is too big");
        }

        return square(screen_size[0] * T{0.5} / sin_alpha);
}
}

template <std::size_t N, typename T>
const std::array<int, N - 1>& SphericalProjector<N, T>::screen_size() const
{
        return screen_size_;
}

template <std::size_t N, typename T>
numerical::Ray<N, T> SphericalProjector<N, T>::ray(const numerical::Vector<N - 1, T>& point) const
{
        const numerical::Vector<N - 1, T> screen_point = screen_org_ + point;

        const T radicand = square_radius_ - screen_point.norm_squared();
        if (!(radicand > 0))
        {
                error("Error spherical projection point " + to_string(point));
        }

        const T z = std::sqrt(radicand);
        const numerical::Vector<N, T> screen_dir = com::screen_dir(screen_axes_, screen_point);

        return numerical::Ray<N, T>(camera_org_, camera_dir_ * z + screen_dir);
}

template <std::size_t N, typename T>
SphericalProjector<N, T>::SphericalProjector(
        const numerical::Vector<N, T>& camera_org,
        const numerical::Vector<N, T>& camera_dir,
        const std::array<numerical::Vector<N, T>, N - 1>& screen_axes,
        const std::type_identity_t<T> width_view_angle_degrees,
        const std::array<int, N - 1>& screen_size)
        : screen_size_(screen_size),
          screen_axes_(com::normalize_axes(screen_axes)),
          screen_org_(com::screen_org<T>(screen_size)),
          camera_org_(camera_org),
          camera_dir_(camera_dir.normalized()),
          square_radius_(make_square_radius<T>(width_view_angle_degrees, screen_size))
{
        com::check_orthogonality(camera_dir_, screen_axes_);
}

#define TEMPLATE(N, T) template class SphericalProjector<(N), T>;

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
