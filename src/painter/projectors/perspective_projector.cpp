/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "perspective_projector.h"

#include "com/functions.h"

#include <src/com/constant.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
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
template <std::size_t N, typename T>
numerical::Vector<N, T> make_camera_dir(
        const numerical::Vector<N, T>& camera_dir,
        const T width_view_angle_degrees,
        const std::array<int, N - 1>& screen_size)
{
        if (!(width_view_angle_degrees > 0 && width_view_angle_degrees < 180))
        {
                error("Perspective projection: error view angle " + to_string(width_view_angle_degrees));
        }

        const T half_angle = degrees_to_radians(width_view_angle_degrees * T{0.5});
        const T dir_length = screen_size[0] * T{0.5} * std::tan(PI<T> / 2 - half_angle);

        return camera_dir.normalized() * dir_length;
}
}

template <std::size_t N, typename T>
const std::array<int, N - 1>& PerspectiveProjector<N, T>::screen_size() const
{
        return screen_size_;
}

template <std::size_t N, typename T>
numerical::Ray<N, T> PerspectiveProjector<N, T>::ray(const numerical::Vector<N - 1, T>& point) const
{
        const numerical::Vector<N - 1, T> screen_point = screen_org_ + point;
        const numerical::Vector<N, T> screen_dir = com::screen_dir(screen_axes_, screen_point);
        return numerical::Ray<N, T>(camera_org_, camera_dir_ + screen_dir);
}

template <std::size_t N, typename T>
PerspectiveProjector<N, T>::PerspectiveProjector(
        const numerical::Vector<N, T>& camera_org,
        const numerical::Vector<N, T>& camera_dir,
        const std::array<numerical::Vector<N, T>, N - 1>& screen_axes,
        const std::type_identity_t<T> width_view_angle_degrees,
        const std::array<int, N - 1>& screen_size)
        : screen_size_(screen_size),
          screen_axes_(com::normalize_axes(screen_axes)),
          screen_org_(com::screen_org<T>(screen_size)),
          camera_org_(camera_org),
          camera_dir_(make_camera_dir(camera_dir, width_view_angle_degrees, screen_size))
{
        com::check_orthogonality(camera_dir_, screen_axes_);
}

#define TEMPLATE(N, T) template class PerspectiveProjector<(N), T>;

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
