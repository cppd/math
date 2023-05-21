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

#include "position_data.h"

#include <src/com/error.h>

namespace ns::filter::test
{
template <typename T>
PositionData<T>::PositionData(std::string name, const PositionFilter<T>* const filter)
        : name_(std::move(name)),
          filter_(filter)
{
        ASSERT(filter_);
}

template <typename T>
void PositionData<T>::save_empty()
{
        positions_.emplace_back();
        speed_.emplace_back();
}

template <typename T>
void PositionData<T>::save(const std::size_t index, const SimulatorPoint<2, T>& point)
{
        positions_.push_back(filter_->position());
        speed_.push_back(Vector<2, T>(index, filter_->speed()));

        nees_position_.add(point.position - filter_->position(), filter_->position_p());
}

template <typename T>
std::string PositionData<T>::nees_string() const
{
        return "Position " + name_ + " Position: " + nees_position_.check_string();
}

template <typename T>
const std::vector<std::optional<Vector<2, T>>>& PositionData<T>::positions() const
{
        return positions_;
}

template <typename T>
const std::vector<std::optional<Vector<2, T>>>& PositionData<T>::speed() const
{
        return speed_;
}

template class PositionData<float>;
template class PositionData<double>;
template class PositionData<long double>;
}
