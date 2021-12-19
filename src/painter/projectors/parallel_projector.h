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
#include <src/numerical/vector.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class ParallelProjector final : public Projector<N, T>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        static std::array<Vector<N, T>, N - 1> make_screen_axes(
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                const T& units_per_pixel)
        {
                if (!(units_per_pixel > 0))
                {
                        error("Error units per pixel " + to_string(units_per_pixel) + " for parallel projection");
                }

                std::array<Vector<N, T>, N - 1> res = projectors_implementation::normalize_axes(screen_axes);
                for (std::size_t i = 0; i < res.size(); ++i)
                {
                        res[i] *= units_per_pixel;
                }
                return res;
        }

        std::array<int, N - 1> screen_size_;
        std::array<Vector<N, T>, N - 1> screen_axes_;
        Vector<N - 1, T> screen_org_;
        Vector<N, T> camera_org_;
        Vector<N, T> camera_dir_;

public:
        ParallelProjector(
                const Vector<N, T>& camera_org,
                const Vector<N, T>& camera_dir,
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                const T& units_per_pixel,
                const std::array<int, N - 1>& screen_size)
                : screen_size_(screen_size),
                  screen_axes_(make_screen_axes(screen_axes, units_per_pixel)),
                  screen_org_(projectors_implementation::screen_org<T>(screen_size)),
                  camera_org_(camera_org),
                  camera_dir_(camera_dir.normalized())
        {
                projectors_implementation::check_orthogonality(camera_dir_, screen_axes_);
        }

        const std::array<int, N - 1>& screen_size() const override
        {
                return screen_size_;
        }

        Ray<N, T> ray(const Vector<N - 1, T>& point) const override
        {
                const Vector<N - 1, T> screen_point = screen_org_ + point;
                const Vector<N, T> screen_dir = projectors_implementation::screen_dir(screen_axes_, screen_point);
                return Ray<N, T>(camera_org_ + screen_dir, camera_dir_);
        }
};
}
