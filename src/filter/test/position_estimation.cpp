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

#include "position_estimation.h"

#include "utility.h"

#include <src/com/conversion.h>
#include <src/com/log.h>

namespace ns::filter::test
{
template <typename T>
PositionEstimation<T>::PositionEstimation(
        const T angle_estimation_time_difference,
        const T angle_estimation_variance,
        const std::vector<Position<T>>* const positions)
        : angle_estimation_time_difference_(angle_estimation_time_difference),
          angle_estimation_variance_(angle_estimation_variance),
          positions_(positions)
{
        ASSERT(positions_);
        last_direction_time_ = std::nullopt;
}

template <typename T>
void PositionEstimation<T>::update(const ProcessMeasurement<2, T>& m)
{
        if (m.direction)
        {
                last_direction_ = *m.direction;
                last_direction_time_ = m.time;
        }

        direction_ = last_direction_time_ && (m.time - *last_direction_time_ <= angle_estimation_time_difference_);

        angle_position_index_.reset();

        if (!direction_)
        {
                return;
        }

        T angle_p = angle_estimation_variance_;
        for (std::size_t i = 0; i < positions_->size(); ++i)
        {
                const Position<T>& position = (*positions_)[i];

                LOG(to_string(m.time) + "; " + position.name()
                    + "; angle p = " + to_string(radians_to_degrees(std::sqrt(position.angle_p()))));

                const T position_angle_p = position.angle_p();
                if (position_angle_p < angle_p)
                {
                        angle_position_index_ = i;
                        angle_p = position_angle_p;
                }
        }
}

template <typename T>
bool PositionEstimation<T>::has_value() const
{
        ASSERT(!direction_ || last_direction_);
        return direction_ && angle_position_index_;
}

template <typename T>
PositionEstimate<T> PositionEstimation<T>::value() const
{
        if (!has_value())
        {
                error("Estimation doesn't have value");
        }
        const Position<T>& position = (*positions_)[*angle_position_index_];
        return {.angle = normalize_angle(*last_direction_ - position.angle()),
                .position = position.position(),
                .velocity = position.velocity()};
}

template <typename T>
std::string PositionEstimation<T>::description() const
{
        if (!has_value())
        {
                error("Estimation doesn't have value");
        }

        const Position<T>& position = (*positions_)[*angle_position_index_];
        const T filter_angle = position.angle();

        std::string res;
        res += "estimation:";
        res += "\nfilter = " + position.name();
        res += "\nangle = " + to_string(radians_to_degrees(filter_angle));
        res += "\nangle stddev = " + to_string(radians_to_degrees(std::sqrt(position.angle_p())));
        res += "\nmeasurement: angle = " + to_string(radians_to_degrees(*last_direction_));
        res += "\nangle difference = "
               + to_string(radians_to_degrees(normalize_angle(*last_direction_ - filter_angle)));
        return res;
}

template class PositionEstimation<float>;
template class PositionEstimation<double>;
template class PositionEstimation<long double>;
}
