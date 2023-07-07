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
#include "simulator.h"

#include <src/numerical/vector.h>

#include <optional>
#include <vector>

namespace ns::filter::test
{
template <typename T>
class PositionEstimation final
{
        const T angle_estimation_time_difference_;
        const T angle_estimation_variance_;
        std::optional<T> last_direction_;
        std::optional<T> last_direction_time_;
        const Position<T>* position_ = nullptr;
        bool direction_ = false;

public:
        PositionEstimation(T angle_estimation_time_difference, T angle_estimation_variance);

        void update(const Measurements<2, T>& m, const std::vector<Position<T>>* positions);

        [[nodiscard]] bool has_estimates() const;
        [[nodiscard]] T angle() const;
        [[nodiscard]] const PositionFilter<T>* filter() const;

        [[nodiscard]] std::string description() const;
};

}
