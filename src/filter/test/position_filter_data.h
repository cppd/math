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

#include <src/numerical/vector.h>

#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test
{
template <typename T>
class PositionFilterData final
{
        std::string name_;
        const PositionFilter<T>* filter_;
        std::vector<std::optional<Vector<2, T>>> positions_;
        std::vector<std::optional<Vector<2, T>>> speed_;
        NeesAverage<2, T> nees_position_;

public:
        PositionFilterData(std::string name, const PositionFilter<T>* filter);

        void save_empty();
        void save(std::size_t index, const SimulatorPoint<2, T>& point);

        [[nodiscard]] std::string nees_string() const;
        [[nodiscard]] const std::vector<std::optional<Vector<2, T>>>& positions() const;
        [[nodiscard]] const std::vector<std::optional<Vector<2, T>>>& speed() const;
};
}
