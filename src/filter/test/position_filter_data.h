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

#include "simulator.h"

#include "../nees.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test
{
template <typename T, template <typename> typename Filter>
class PositionFilterData final
{
        std::string name_;
        const Filter<T>* filter_;
        std::vector<std::optional<Vector<2, T>>> positions_;
        NeesAverage<2, T> nees_position_;

public:
        PositionFilterData(std::string name, const Filter<T>* const filter, const std::size_t reserve)
                : name_(std::move(name)),
                  filter_(filter)
        {
                ASSERT(filter_);

                positions_.reserve(reserve);
        }

        void save_empty()
        {
                positions_.emplace_back();
        }

        void save(const SimulatorPoint<2, T>& point)
        {
                positions_.push_back(filter_->position());
                nees_position_.add(point.position, filter_->position(), filter_->position_p());
        }

        [[nodiscard]] std::string nees_string() const
        {
                return "Position " + name_ + " Position: " + nees_position_.check_string();
        }

        [[nodiscard]] const std::vector<std::optional<Vector<2, T>>>& positions() const
        {
                return positions_;
        }
};
}
