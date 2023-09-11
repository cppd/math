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

#include "../utility.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/log.h>

namespace ns::filter::test::position
{
template <typename T>
PositionEstimation<T>::PositionEstimation(
        const T angle_estimation_time_difference,
        const T angle_estimation_variance,
        const Position<2, T>* const position)
        : angle_estimation_time_difference_(angle_estimation_time_difference),
          angle_estimation_variance_(angle_estimation_variance),
          position_(position)
{
}

template <typename T>
void PositionEstimation<T>::update(const Measurements<2, T>& m)
{
        if (m.direction)
        {
                last_direction_ = m.direction->value[0];
                last_direction_time_ = m.time;
        }

        if (!m.position)
        {
                return;
        }

        has_angle_difference_ = false;
        angle_p_.reset();

        if (position_->empty())
        {
                return;
        }

        const T angle_p = compute_angle_p(position_->velocity(), position_->velocity_p());
        if (!is_finite(angle_p))
        {
                return;
        }

        angle_p_ = angle_p;

        LOG(to_string(m.time) + "; " + position_->name()
            + "; angle p = " + to_string(radians_to_degrees(std::sqrt(*angle_p_))));

        has_angle_difference_ =
                last_direction_time_ && (m.time - *last_direction_time_ <= angle_estimation_time_difference_)
                && *angle_p_ <= angle_estimation_variance_;
}

template <typename T>
bool PositionEstimation<T>::has_angle_difference() const
{
        return has_angle_difference_;
}

template <typename T>
T PositionEstimation<T>::angle_difference() const
{
        if (!has_angle_difference())
        {
                error("Estimation doesn't have angle difference");
        }
        ASSERT(last_direction_);
        return normalize_angle(*last_direction_ - compute_angle(position_->velocity()));
}

template <typename T>
bool PositionEstimation<T>::has_angle_p() const
{
        return angle_p_.has_value();
}

template <typename T>
T PositionEstimation<T>::angle_p() const
{
        if (!has_angle_p())
        {
                error("Estimation doesn't have angle p");
        }
        ASSERT(angle_p_);
        return *angle_p_;
}

template <typename T>
Vector<6, T> PositionEstimation<T>::position_velocity_acceleration() const
{
        return position_->position_velocity_acceleration();
}

template <typename T>
Matrix<6, 6, T> PositionEstimation<T>::position_velocity_acceleration_p() const
{
        return position_->position_velocity_acceleration_p();
}

template <typename T>
std::string PositionEstimation<T>::position_description() const
{
        const Vector<2, T> velocity = position_->velocity();
        const Matrix<2, 2, T> velocity_p = position_->velocity_p();
        const T angle = compute_angle(velocity);
        const T angle_p = compute_angle_p(velocity, velocity_p);

        std::string res;
        res += "filter = " + position_->name();
        res += "; angle = " + to_string(radians_to_degrees(angle));
        res += "; angle stddev = " + to_string(radians_to_degrees(std::sqrt(angle_p)));
        return res;
}

template <typename T>
std::string PositionEstimation<T>::angle_difference_description() const
{
        if (!has_angle_difference())
        {
                error("Estimation doesn't have angle difference");
        }

        ASSERT(last_direction_);

        const Vector<2, T> velocity = position_->velocity();
        const Matrix<2, 2, T> velocity_p = position_->velocity_p();
        const T angle = compute_angle(velocity);
        const T angle_p = compute_angle_p(velocity, velocity_p);

        std::string res;
        res += "filter = " + position_->name();
        res += "; angle = " + to_string(radians_to_degrees(angle));
        res += "; angle stddev = " + to_string(radians_to_degrees(std::sqrt(angle_p)));
        res += "; measurement: angle = " + to_string(radians_to_degrees(*last_direction_));
        res += "; angle difference = " + to_string(radians_to_degrees(normalize_angle(*last_direction_ - angle)));
        return res;
}

template class PositionEstimation<float>;
template class PositionEstimation<double>;
template class PositionEstimation<long double>;
}
