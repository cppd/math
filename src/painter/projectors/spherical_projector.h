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
// Project screen points onto a hemisphere with the center on the screen.
// Make rays from the hemisphere center to the projections.
template <std::size_t N, typename T>
class SphericalProjector final : public Projector<N, T>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        std::array<int, N - 1> screen_size_;
        std::array<numerical::Vector<N, T>, N - 1> screen_axes_;
        numerical::Vector<N - 1, T> screen_org_;
        numerical::Vector<N, T> camera_org_;
        numerical::Vector<N, T> camera_dir_;
        T square_radius_;

        [[nodiscard]] const std::array<int, N - 1>& screen_size() const override;

        [[nodiscard]] numerical::Ray<N, T> ray(const numerical::Vector<N - 1, T>& point) const override;

public:
        SphericalProjector(
                const numerical::Vector<N, T>& camera_org,
                const numerical::Vector<N, T>& camera_dir,
                const std::array<numerical::Vector<N, T>, N - 1>& screen_axes,
                std::type_identity_t<T> width_view_angle_degrees,
                const std::array<int, N - 1>& screen_size);
};
}
