/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "position_2.h"

#include "../estimation.h"
#include "../measurement.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::test::filter::position
{
template <typename T>
class PositionEstimation final : public Estimation<T>
{
        const Position2<2, T>* const position_;
        std::optional<T> angle_p_;

public:
        explicit PositionEstimation(const Position2<2, T>* position);

        void update(const Measurements<2, T>& m);

        [[nodiscard]] bool angle_p_less_than(T p) const override;

        [[nodiscard]] Vector<2, T> position() const override;
        [[nodiscard]] Matrix<2, 2, T> position_p() const override;
        [[nodiscard]] Vector<2, T> velocity() const override;
        [[nodiscard]] Vector<4, T> position_velocity() const override;
        [[nodiscard]] Matrix<4, 4, T> position_velocity_p() const override;
        [[nodiscard]] Vector<6, T> position_velocity_acceleration() const override;
        [[nodiscard]] Matrix<6, 6, T> position_velocity_acceleration_p() const override;
};
}
