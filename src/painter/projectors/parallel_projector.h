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

#pragma once

#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>

#include <array>
#include <cstddef>
#include <type_traits>

namespace ns::painter::projectors
{
template <std::size_t N, typename T>
class ParallelProjector final : public Projector<N, T>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        std::array<int, N - 1> screen_size_;
        std::array<Vector<N, T>, N - 1> screen_axes_;
        Vector<N - 1, T> screen_org_;
        Vector<N, T> camera_org_;
        Vector<N, T> camera_dir_;

        [[nodiscard]] const std::array<int, N - 1>& screen_size() const override;

        [[nodiscard]] Ray<N, T> ray(const Vector<N - 1, T>& point) const override;

public:
        ParallelProjector(
                const Vector<N, T>& camera_org,
                const Vector<N, T>& camera_dir,
                const std::array<Vector<N, T>, N - 1>& screen_axes,
                std::type_identity_t<T> units_per_pixel,
                const std::array<int, N - 1>& screen_size);
};
}
