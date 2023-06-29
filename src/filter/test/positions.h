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
class Positions final
{
        const T angle_estimation_time_difference_;
        const T angle_estimation_variance_;
        std::vector<Position<T>> positions_;
        std::optional<T> last_direction_;
        std::optional<T> last_direction_time_;
        std::optional<std::size_t> angle_position_index_;
        bool direction_ = false;

        [[nodiscard]] std::string description() const;

public:
        Positions(T angle_estimation_time_difference, T angle_estimation_variance, std::vector<Position<T>> positions);

        void update(const Measurement<2, T>& m);

        [[nodiscard]] const std::vector<Position<T>>& positions() const;
        [[nodiscard]] bool has_estimates() const;
        [[nodiscard]] T angle() const;
        [[nodiscard]] Vector<2, T> position() const;
        [[nodiscard]] Vector<2, T> velocity() const;
};

}
