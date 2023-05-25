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
Position<T>::Position(PositionFilter<T>&& filter)
        : filter_(std::move(filter))
{
}

template <typename T>
void Position<T>::save(const T time, const SimulatorPoint<2, T>& point)
{
        const Vector<2, T> p = filter_.position();
        position_.push_back({time, p[0], p[1]});
        speed_.push_back({time, filter_.speed()});

        nees_position_.add(point.position - filter_.position(), filter_.position_p());
}

template <typename T>
void Position<T>::update(
        const PositionMeasurement<2, T>& measurement,
        const T position_measurement_variance,
        const SimulatorPoint<2, T>& point)
{
        ASSERT(!last_time_ || *last_time_ < measurement.time);

        const T delta = last_time_ ? (measurement.time - *last_time_) : 0;
        last_time_ = measurement.time;

        filter_.predict(delta);
        filter_.update(measurement.position, position_measurement_variance);

        save(measurement.time, point);
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
const std::vector<Vector<3, T>>& Position<T>::positions() const
{
        return position_;
}

template <typename T>
const std::vector<Vector<2, T>>& Position<T>::speed() const
{
        return speed_;
}

template class Position<float>;
template class Position<double>;
template class Position<long double>;
}
