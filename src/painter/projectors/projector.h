/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

namespace painter
{
namespace projector_implementation
{
template <size_t N, typename T>
void check_vectors_orthogonal(const Vector<N, T>& camera_dir, const std::array<Vector<N, T>, N - 1>& screen_axes)
{
        constexpr T LIMIT_COS = limits<T>::epsilon() * 100;

        for (unsigned i = 0; i < N - 1; ++i)
        {
                if (!(std::abs(dot(screen_axes[i], camera_dir)) <= LIMIT_COS))
                {
                        error("The screen axis " + to_string(i) + " is not orthogonal to the camera direction");
                }

                for (unsigned j = i + 1; j < N - 1; ++j)
                {
                        if (!(std::abs(dot(screen_axes[i], screen_axes[j])) <= LIMIT_COS))
                        {
                                error("The screen axis " + to_string(i) + " is not orthogonal to the screen axes "
                                      + to_string(j));
                        }
                }
        }
}

template <typename T, size_t N>
Vector<N, T> screen_org(const std::array<int, N>& sizes)
{
        Vector<N, T> org;

        for (unsigned i = 0; i < N; ++i)
        {
                if (sizes[i] < 1)
                {
                        error("Projection size " + to_string(i) + " is not positive (" + to_string(sizes[i]) + ")");
                }
                org[i] = -sizes[i] * (static_cast<T>(1) / 2);
        }

        return org;
}

template <size_t N, typename T>
std::tuple<Vector<N, T>, std::array<Vector<N, T>, N - 1>> unit_dir_and_axes(
        const Vector<N, T>& camera_dir,
        const std::array<Vector<N, T>, N - 1>& screen_axes)
{
        std::tuple<Vector<N, T>, std::array<Vector<N, T>, N - 1>> res;

        std::get<0>(res) = camera_dir.normalized();

        for (unsigned i = 0; i < N - 1; ++i)
        {
                std::get<1>(res)[i] = screen_axes[i].normalized();
        }

        check_vectors_orthogonal(std::get<0>(res), std::get<1>(res));

        return res;
}

template <size_t N, typename T>
Vector<N, T> compute_screen_dir(
        const std::array<Vector<N, T>, N - 1>& screen_axes,
        const Vector<N - 1, T>& screen_point)
{
        Vector<N, T> screen_dir;

        screen_dir = screen_axes[0] * screen_point[0];
        for (unsigned i = 1; i < N - 1; ++i)
        {
                screen_dir += screen_axes[i] * screen_point[i];
        }

        return screen_dir;
}
}

template <size_t N, typename T>
class PerspectiveProjector
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        std::array<int, N - 1> m_screen_size;
        std::array<Vector<N, T>, N - 1> m_screen_axes;
        Vector<N - 1, T> m_screen_org;
        Vector<N, T> m_camera_org, m_camera_dir;

public:
        PerspectiveProjector(
                const Vector<N, T>& camera_org,
                const Vector<N, T>& camera_dir,
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                T width_view_angle_degrees,
                const std::array<int, N - 1>& screen_size)
        {
                namespace impl = projector_implementation;

                m_screen_size = screen_size;
                m_screen_org = impl::screen_org<T>(screen_size);
                m_camera_org = camera_org;
                std::tie(m_camera_dir, m_screen_axes) = impl::unit_dir_and_axes(camera_dir, screen_axes);

                //

                if (!(width_view_angle_degrees > 0 && width_view_angle_degrees < 180))
                {
                        error("Perspective projection: error view angle " + to_string(width_view_angle_degrees));
                }

                T half_angle = width_view_angle_degrees * T(0.5) * PI<T> / 180;
                T dir_length = screen_size[0] * T(0.5) * std::tan(PI<T> / 2 - half_angle);

                m_camera_dir *= dir_length;
        }

        const std::array<int, N - 1>& screen_size() const
        {
                return m_screen_size;
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const
        {
                namespace impl = projector_implementation;

                Vector<N - 1, T> screen_point = m_screen_org + point;
                Vector<N, T> screen_dir = impl::compute_screen_dir(m_screen_axes, screen_point);
                return Ray<N, T>(m_camera_org, m_camera_dir + screen_dir);
        }
};

template <size_t N, typename T>
class ParallelProjector
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        std::array<int, N - 1> m_screen_size;
        std::array<Vector<N, T>, N - 1> m_screen_axes;
        Vector<N - 1, T> m_screen_org;
        Vector<N, T> m_camera_org, m_camera_dir;

public:
        ParallelProjector(
                const Vector<N, T>& camera_org,
                const Vector<N, T>& camera_dir,
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                T units_per_pixel,
                const std::array<int, N - 1>& screen_size)
        {
                namespace impl = projector_implementation;

                m_screen_size = screen_size;
                m_screen_org = impl::screen_org<T>(screen_size);
                m_camera_org = camera_org;
                std::tie(m_camera_dir, m_screen_axes) = impl::unit_dir_and_axes(camera_dir, screen_axes);

                //

                if (!(units_per_pixel > 0))
                {
                        error("Error units per pixel for parallel projection");
                }

                for (unsigned i = 0; i < N - 1; ++i)
                {
                        m_screen_axes[i] *= units_per_pixel;
                }
        }

        const std::array<int, N - 1>& screen_size() const
        {
                return m_screen_size;
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const
        {
                namespace impl = projector_implementation;

                Vector<N - 1, T> screen_point = m_screen_org + point;
                Vector<N, T> screen_dir = impl::compute_screen_dir(m_screen_axes, screen_point);
                return Ray<N, T>(m_camera_org + screen_dir, m_camera_dir);
        }
};

// Параллельное проецирование точек экрана на полусферу и создание лучей
// из центра полусферы в направлении точек на сфере.
template <size_t N, typename T>
class SphericalProjector
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        std::array<int, N - 1> m_screen_size;
        std::array<Vector<N, T>, N - 1> m_screen_axes;
        Vector<N - 1, T> m_screen_org;
        Vector<N, T> m_camera_org, m_camera_dir;

        T m_square_radius;

public:
        SphericalProjector(
                const Vector<N, T>& camera_org,
                const Vector<N, T>& camera_dir,
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                T width_view_angle_degrees,
                const std::array<int, N - 1>& screen_size)
        {
                namespace impl = projector_implementation;

                m_screen_size = screen_size;
                m_screen_org = impl::screen_org<T>(screen_size);
                m_camera_org = camera_org;
                std::tie(m_camera_dir, m_screen_axes) = impl::unit_dir_and_axes(camera_dir, screen_axes);

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

                m_square_radius = square(screen_size[0] * T(0.5) / sin_alpha);
        }

        const std::array<int, N - 1>& screen_size() const
        {
                return m_screen_size;
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const
        {
                namespace impl = projector_implementation;

                Vector<N - 1, T> screen_point = m_screen_org + point;

                T radicand = m_square_radius - dot(screen_point, screen_point);
                if (!(radicand > 0))
                {
                        error("Error spherical projection radicand");
                }
                T z = std::sqrt(radicand);

                Vector<N, T> screen_dir = impl::compute_screen_dir(m_screen_axes, screen_point);
                return Ray<N, T>(m_camera_org, m_camera_dir * z + screen_dir);
        }
};
}
