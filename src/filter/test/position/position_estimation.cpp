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
        const Position2<2, T>* const position)
        : angle_estimation_time_difference_(angle_estimation_time_difference),
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
        else
        {
                ASSERT(!last_direction_time_ || m.time >= *last_direction_time_);
        }

        angle_p_.reset();
        measurement_angle_.reset();

        if (last_direction_time_ && (m.time - *last_direction_time_ <= angle_estimation_time_difference_))
        {
                ASSERT(last_direction_);
                measurement_angle_ = *last_direction_;
        }

        if (!m.position)
        {
                return;
        }

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
}

template <typename T>
std::optional<T> PositionEstimation<T>::measurement_angle() const
{
        return measurement_angle_;
}

template <typename T>
bool PositionEstimation<T>::has_angle() const
{
        return angle_p_.has_value();
}

template <typename T>
T PositionEstimation<T>::angle() const
{
        if (!has_angle())
        {
                error("Estimation doesn't have angle");
        }
        return compute_angle(position_->velocity());
}

template <typename T>
T PositionEstimation<T>::angle_p() const
{
        if (!has_angle())
        {
                error("Estimation doesn't have angle");
        }
        ASSERT(angle_p_);
        return *angle_p_;
}

template <typename T>
Vector<2, T> PositionEstimation<T>::position() const
{
        return position_->position();
}

template <typename T>
Matrix<2, 2, T> PositionEstimation<T>::position_p() const
{
        return position_->position_p();
}

template <typename T>
Vector<4, T> PositionEstimation<T>::position_velocity() const
{
        return position_->position_velocity();
}

template <typename T>
Matrix<4, 4, T> PositionEstimation<T>::position_velocity_p() const
{
        return position_->position_velocity_p();
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
std::string PositionEstimation<T>::description() const
{
        const Vector<2, T> velocity = position_->velocity();
        const Matrix<2, 2, T> velocity_p = position_->velocity_p();
        const T angle = compute_angle(velocity);
        const T angle_p = compute_angle_p(velocity, velocity_p);

        std::string res;

        res += "filter = " + position_->name();
        res += "; angle = " + to_string(radians_to_degrees(angle));
        res += "; angle stddev = " + to_string(radians_to_degrees(std::sqrt(angle_p)));

        if (measurement_angle_)
        {
                res += "; measurement angle = " + to_string(radians_to_degrees(*measurement_angle_));
                res += "; angle difference = "
                       + to_string(radians_to_degrees(normalize_angle(*measurement_angle_ - angle)));
        }

        return res;
}

template class PositionEstimation<float>;
template class PositionEstimation<double>;
template class PositionEstimation<long double>;
}
