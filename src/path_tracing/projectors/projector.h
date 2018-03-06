/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "path_tracing/objects.h"

#include "com/error.h"
#include "com/math.h"
#include "com/print.h"
#include "com/types.h"
#include "com/vec.h"

namespace ProjectorImplementation
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
                                error("The screen axis " + to_string(i) + " is not orthogonal to the screen axes " +
                                      to_string(j));
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
std::tuple<Vector<N, T>, std::array<Vector<N, T>, N - 1>> unit_dir_and_axes(const Vector<N, T>& camera_dir,
                                                                            const std::array<Vector<N, T>, N - 1>& screen_axes)
{
        std::tuple<Vector<N, T>, std::array<Vector<N, T>, N - 1>> res;

        std::get<0>(res) = normalize(camera_dir);
        for (unsigned i = 0; i < N - 1; ++i)
        {
                std::get<1>(res)[i] = normalize(screen_axes[i]);
        }
        check_vectors_orthogonal(std::get<0>(res), std::get<1>(res));

        return res;
}
}

class PerspectiveProjector final : public Projector
{
        std::array<int, 2> m_screen_sizes;
        std::array<vec3, 2> m_screen_axes;
        vec2 m_screen_org;
        vec3 m_camera_org, m_camera_dir;

public:
        PerspectiveProjector(const vec3& camera_org, const vec3& camera_dir, const std::array<vec3, 2>& screen_axes,
                             double width_view_angle_degrees, const std::array<int, 2>& sizes)
        {
                namespace Impl = ProjectorImplementation;

                m_screen_sizes = sizes;
                m_screen_org = Impl::screen_org<double>(sizes);

                if (!(width_view_angle_degrees > 0 && width_view_angle_degrees < 180))
                {
                        error("Perspective projection: error view angle " + to_string(width_view_angle_degrees));
                }

                m_camera_org = camera_org;
                std::tie(m_camera_dir, m_screen_axes) = Impl::unit_dir_and_axes(camera_dir, screen_axes);

                //

                double half_angle = width_view_angle_degrees * 0.5 / 180.0 * PI<double>;
                double dir_length = sizes[0] * 0.5 * std::tan(0.5 * PI<double> - half_angle);

                m_camera_dir *= dir_length;
        }

        int screen_width() const override
        {
                return m_screen_sizes[0];
        }

        int screen_height() const override
        {
                return m_screen_sizes[1];
        }

        ray3 ray(const vec2& point) const override
        {
                vec2 screen_point = m_screen_org + point;

                vec3 screen_dir = m_screen_axes[0] * screen_point[0];
                for (unsigned i = 1; i < 2; ++i)
                {
                        screen_dir += m_screen_axes[i] * screen_point[i];
                }

                return ray3(m_camera_org, m_camera_dir + screen_dir);
        }
};

class ParallelProjector final : public Projector
{
        std::array<int, 2> m_screen_sizes;
        std::array<vec3, 2> m_screen_axes;
        vec2 m_screen_org;
        vec3 m_camera_org, m_camera_dir;

public:
        ParallelProjector(const vec3& camera_org, const vec3& camera_dir, const std::array<vec3, 2>& screen_axes,
                          double view_width, const std::array<int, 2>& sizes)
        {
                namespace Impl = ProjectorImplementation;

                m_screen_sizes = sizes;
                m_screen_org = Impl::screen_org<double>(sizes);

                if (!(view_width > 0))
                {
                        error("Error view width for parallel projection");
                }

                m_camera_org = camera_org;
                std::tie(m_camera_dir, m_screen_axes) = Impl::unit_dir_and_axes(camera_dir, screen_axes);

                //

                double coef = view_width / sizes[0];
                m_screen_axes[0] *= coef;
                m_screen_axes[1] *= coef;
        }

        int screen_width() const override
        {
                return m_screen_sizes[0];
        }

        int screen_height() const override
        {
                return m_screen_sizes[1];
        }

        ray3 ray(const vec2& point) const override
        {
                vec2 screen_point = m_screen_org + point;

                vec3 screen_dir = m_screen_axes[0] * screen_point[0];
                for (unsigned i = 1; i < 2; ++i)
                {
                        screen_dir += m_screen_axes[i] * screen_point[i];
                }

                return ray3(m_camera_org + screen_dir, m_camera_dir);
        }
};

// Параллельное проецирование точек экрана на полусферу и создание лучей
// из центра полусферы в направлении точек на сфере.
class SphericalProjector final : public Projector
{
        std::array<int, 2> m_screen_sizes;
        std::array<vec3, 2> m_screen_axes;
        vec2 m_screen_org;
        vec3 m_camera_org, m_camera_dir;

        double m_square_radius;

public:
        SphericalProjector(const vec3& camera_org, const vec3& camera_dir, const std::array<vec3, 2>& screen_axes,
                           double width_view_angle_degrees, const std::array<int, 2>& sizes)
        {
                namespace Impl = ProjectorImplementation;

                m_screen_sizes = sizes;
                m_screen_org = Impl::screen_org<double>(sizes);

                double half_angle = width_view_angle_degrees * 0.5 / 180.0 * PI<double>;

                double sin_alpha = std::sin(half_angle);

                if (!(width_view_angle_degrees > 0 && (square(sin_alpha) + square((sin_alpha / sizes[0]) * sizes[1]) < 1)))
                {
                        error("Error view angle for spherical projection");
                }

                m_square_radius = square(sizes[0] * 0.5 / sin_alpha);

                m_camera_org = camera_org;
                std::tie(m_camera_dir, m_screen_axes) = Impl::unit_dir_and_axes(camera_dir, screen_axes);
        }

        int screen_width() const override
        {
                return m_screen_sizes[0];
        }

        int screen_height() const override
        {
                return m_screen_sizes[1];
        }

        ray3 ray(const vec2& point) const override
        {
                vec2 screen_point = m_screen_org + point;

                double radicand = m_square_radius - square(screen_point[0]) - square(screen_point[1]);
                if (!(radicand > 0))
                {
                        error("Error spherical projection radicand");
                }

                double z = std::sqrt(radicand);

                vec3 screen_dir = m_screen_axes[0] * screen_point[0];
                for (unsigned i = 1; i < 2; ++i)
                {
                        screen_dir += m_screen_axes[i] * screen_point[i];
                }

                return ray3(m_camera_org, m_camera_dir * z + screen_dir);
        }
};
