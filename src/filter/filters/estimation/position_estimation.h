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

#include "../estimation.h"
#include "../measurement.h"
#include "../position/position_2.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::filter::filters::estimation
{
template <std::size_t N, typename T>
class PositionEstimation final : public Estimation<N, T>
{
        const position::Position2<N, T>* const position_;
        std::optional<Vector<N, T>> angle_variance_;

public:
        explicit PositionEstimation(const position::Position2<N, T>* position);

        void update(const Measurements<N, T>& m);

        [[nodiscard]] bool angle_variance_less_than(T variance) const override;

        [[nodiscard]] Vector<2 * N, T> position_velocity() const override;
        [[nodiscard]] Matrix<2 * N, 2 * N, T> position_velocity_p() const override;
};
}
