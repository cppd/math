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

#include "objects.h"

#include "painter/projectors/projector.h"

template <size_t N, typename T>
class VisiblePerspectiveProjector final : public Projector<N, T>
{
        PerspectiveProjector<N, T> m_projector;

public:
        VisiblePerspectiveProjector(const Vector<N, T>& camera_org, const Vector<N, T>& camera_dir,
                                    const std::array<Vector<N, T>, N - 1>& screen_axes, T width_view_angle_degrees,
                                    const std::array<int, N - 1>& screen_size)
                : m_projector(camera_org, camera_dir, screen_axes, width_view_angle_degrees, screen_size)
        {
        }

        const std::array<int, N - 1>& screen_size() const override
        {
                return m_projector.screen_size();
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const override
        {
                return m_projector.ray(point);
        }
};

template <size_t N, typename T>
class VisibleParallelProjector final : public Projector<N, T>
{
        ParallelProjector<N, T> m_projector;

public:
        VisibleParallelProjector(const Vector<N, T>& camera_org, const Vector<N, T>& camera_dir,
                                 const std::array<Vector<N, T>, N - 1>& screen_axes, T units_per_pixel,
                                 const std::array<int, N - 1>& screen_size)
                : m_projector(camera_org, camera_dir, screen_axes, units_per_pixel, screen_size)
        {
        }

        const std::array<int, N - 1>& screen_size() const override
        {
                return m_projector.screen_size();
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const override
        {
                return m_projector.ray(point);
        }
};

template <size_t N, typename T>
class VisibleSphericalProjector final : public Projector<N, T>
{
        SphericalProjector<N, T> m_projector;

public:
        VisibleSphericalProjector(const Vector<N, T>& camera_org, const Vector<N, T>& camera_dir,
                                  const std::array<Vector<N, T>, N - 1>& screen_axes, T width_view_angle_degrees,
                                  const std::array<int, N - 1>& screen_size)
                : m_projector(camera_org, camera_dir, screen_axes, width_view_angle_degrees, screen_size)
        {
        }

        const std::array<int, N - 1>& screen_size() const override
        {
                return m_projector.screen_size();
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const override
        {
                return m_projector.ray(point);
        }
};
