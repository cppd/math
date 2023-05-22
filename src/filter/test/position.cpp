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

#include "position.h"

#include <src/com/error.h>

namespace ns::filter::test
{
template <typename T>
Position<T>::Position(PositionFilter<T>&& filter, const T dt, const T position_interval)
        : filter_(std::move(filter)),
          dt_(dt),
          position_interval_(position_interval)
{
}

template <typename T>
void Position<T>::update(
        const PositionMeasurement<2, T>& measurement,
        const T position_measurement_variance,
        const SimulatorPoint<2, T>& point)
{
        ASSERT(!last_position_i_ || *last_position_i_ < measurement.index);

        const std::size_t delta = last_position_i_ ? (measurement.index - *last_position_i_) : 1;

        if (delta > position_interval_)
        {
                positions_.emplace_back();
                speed_.emplace_back();
        }

        if (delta > 0)
        {
                filter_.predict(delta * dt_);
                filter_.update(measurement.position, position_measurement_variance);

                positions_.push_back(filter_.position());
                speed_.push_back(Vector<2, T>(measurement.index, filter_.speed()));

                nees_position_.add(point.position - filter_.position(), filter_.position_p());
        }

        last_position_i_ = measurement.index;
}

template <typename T>
const PositionFilter<T>& Position<T>::filter() const
{
        return filter_;
}

template <typename T>
std::string Position<T>::nees_string() const
{
        return "Position Filter: " + nees_position_.check_string();
}

template <typename T>
const std::vector<std::optional<Vector<2, T>>>& Position<T>::positions() const
{
        return positions_;
}

template <typename T>
const std::vector<std::optional<Vector<2, T>>>& Position<T>::speed() const
{
        return speed_;
}

template class Position<float>;
template class Position<double>;
template class Position<long double>;
}
