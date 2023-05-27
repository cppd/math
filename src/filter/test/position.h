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

#include "position_filter.h"
#include "simulator.h"

#include "../nees.h"

#include <src/color/rgb8.h>
#include <src/numerical/vector.h>

#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test
{
template <typename T>
class Position final
{
        std::string name_;
        color::RGB8 color_;
        PositionFilter<T> filter_;

        std::vector<Vector<3, T>> position_;
        std::vector<Vector<2, T>> speed_;
        NeesAverage<2, T> nees_position_;

        std::optional<T> last_time_;

        void save(T time, const SimulatorPoint<2, T>& point);

public:
        Position(std::string name, color::RGB8 color, PositionFilter<T>&& filter);

        void update(
                const ProcessMeasurement<2, T>& measurement,
                T position_measurement_variance,
                const SimulatorPoint<2, T>& point);

        [[nodiscard]] const PositionFilter<T>& filter() const;

        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] color::RGB8 color() const;

        [[nodiscard]] std::string nees_string() const;
        [[nodiscard]] const std::vector<Vector<3, T>>& position() const;
        [[nodiscard]] const std::vector<Vector<2, T>>& speed() const;
};
}
