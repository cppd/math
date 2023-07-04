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

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/log.h>

namespace ns::filter::test
{
template <typename T>
PositionEstimation<T>::PositionEstimation(const T angle_estimation_time_difference, const T angle_estimation_variance)
        : angle_estimation_time_difference_(angle_estimation_time_difference),
          angle_estimation_variance_(angle_estimation_variance)
{
}

template <typename T>
void PositionEstimation<T>::update(const Measurement<2, T>& m, const std::vector<Position<T>>* const positions)
{
        if (m.direction)
        {
                last_direction_ = *m.direction;
                last_direction_time_ = m.time;
        }

        if (!m.position)
        {
                return;
        }

        direction_ = last_direction_time_ && (m.time - *last_direction_time_ <= angle_estimation_time_difference_);

        position_ = nullptr;

        if (!direction_)
        {
                return;
        }

        T angle_p = angle_estimation_variance_;
        for (std::size_t i = 0; i < positions->size(); ++i)
        {
                const Position<T>& position = (*positions)[i];

                LOG(to_string(m.time) + "; " + position.name()
                    + "; angle p = " + to_string(radians_to_degrees(std::sqrt(position.filter()->angle_p()))));

                const T position_angle_p = position.filter()->angle_p();
                if (position_angle_p < angle_p)
                {
                        position_ = &position;
                        angle_p = position_angle_p;
                }
        }
}

template <typename T>
bool PositionEstimation<T>::has_estimates() const
{
        ASSERT(!direction_ || last_direction_);

        return last_direction_ && direction_ && position_;
}

template <typename T>
T PositionEstimation<T>::angle() const
{
        if (!has_estimates())
        {
                error("Estimation doesn't have angle");
        }
        return normalize_angle(*last_direction_ - position_->filter()->angle());
}

template <typename T>
const PositionFilter<T>* PositionEstimation<T>::filter() const
{
        if (!has_estimates())
        {
                error("Estimation doesn't have filter");
        }
        return position_->filter();
}

template <typename T>
std::string PositionEstimation<T>::description() const
{
        if (!has_estimates())
        {
                error("Estimation doesn't have description");
        }

        const T filter_angle = position_->filter()->angle();

        std::string res;
        res += "filter = " + position_->name();
        res += "; angle = " + to_string(radians_to_degrees(filter_angle));
        res += "; angle stddev = " + to_string(radians_to_degrees(std::sqrt(position_->filter()->angle_p())));
        res += "; measurement: angle = " + to_string(radians_to_degrees(*last_direction_));
        res += "; angle difference = "
               + to_string(radians_to_degrees(normalize_angle(*last_direction_ - filter_angle)));
        return res;
}

template class PositionEstimation<float>;
template class PositionEstimation<double>;
template class PositionEstimation<long double>;
}
