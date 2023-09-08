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

#include "position.h"

#include "../estimation.h"
#include "../measurement.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>
#include <vector>

namespace ns::filter::test::position
{
template <typename T>
class PositionEstimation final : public Estimation<T>
{
        const T angle_estimation_time_difference_;
        const T angle_estimation_variance_;
        std::optional<T> last_direction_;
        std::optional<T> last_direction_time_;
        const Position<2, T>* position_ = nullptr;
        std::optional<T> position_angle_p_;
        const Position<2, T>* angle_difference_position_ = nullptr;

public:
        PositionEstimation(T angle_estimation_time_difference, T angle_estimation_variance);

        void update(const Measurements<2, T>& m, const std::vector<Position<2, T>>* positions);

        [[nodiscard]] bool has_angle_difference() const override;
        [[nodiscard]] T angle_difference() const override;

        [[nodiscard]] bool has_position() const override;
        [[nodiscard]] T position_angle_p() const override;
        [[nodiscard]] Vector<6, T> position_velocity_acceleration() const override;
        [[nodiscard]] Matrix<6, 6, T> position_velocity_acceleration_p() const override;

        [[nodiscard]] std::string position_description() const override;
        [[nodiscard]] std::string angle_difference_description() const override;
};
}
