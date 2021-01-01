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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class ParallelProjector final : public Projector<N, T>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        std::array<int, N - 1> m_screen_size;
        std::array<Vector<N, T>, N - 1> m_screen_axes;
        Vector<N - 1, T> m_screen_org;
        Vector<N, T> m_camera_org;
        Vector<N, T> m_camera_dir;

public:
        ParallelProjector(
                const Vector<N, T>& camera_org,
                const Vector<N, T>& camera_dir,
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                T units_per_pixel,
                const std::array<int, N - 1>& screen_size)
        {
                m_screen_size = screen_size;
                m_screen_org = projectors_implementation::screen_org<T>(screen_size);
                m_camera_org = camera_org;
                m_camera_dir = camera_dir.normalized();
                m_screen_axes = projectors_implementation::normalize_axes(screen_axes);

                projectors_implementation::check_orthogonality(m_camera_dir, m_screen_axes);

                //

                if (!(units_per_pixel > 0))
                {
                        error("Error units per pixel " + to_string(units_per_pixel) + " for parallel projection");
                }

                for (unsigned i = 0; i < N - 1; ++i)
                {
                        m_screen_axes[i] *= units_per_pixel;
                }
        }

        const std::array<int, N - 1>& screen_size() const override
        {
                return m_screen_size;
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const override
        {
                Vector<N - 1, T> screen_point = m_screen_org + point;
                Vector<N, T> screen_dir = projectors_implementation::screen_dir(m_screen_axes, screen_point);
                return Ray<N, T>(m_camera_org + screen_dir, m_camera_dir);
        }
};
}
