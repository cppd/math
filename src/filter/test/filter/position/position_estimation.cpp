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

#include "../../utility/utility.h"

#include <src/com/conversion.h>
#include <src/com/log.h>

namespace ns::filter::test::filter::position
{
namespace
{
template <std::size_t N, typename T>
Vector<N, T> stddev_degrees(const Vector<N, T>& v)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = radians_to_degrees(std::sqrt(v[i]));
        }
        return res;
}
}

template <typename T>
PositionEstimation<T>::PositionEstimation(const Position2<2, T>* const position)
        : position_(position)
{
}

template <typename T>
void PositionEstimation<T>::update(const Measurements<2, T>& m)
{
        angle_variance_.reset();

        if (!m.position)
        {
                return;
        }

        if (position_->empty())
        {
                return;
        }

        const Vector<2, T> angle_variance =
                utility::compute_angle_variance(position_->velocity(), position_->velocity_p());
        if (!is_finite(angle_variance))
        {
                return;
        }
        ASSERT(angle_variance[0] == angle_variance[1]);

        angle_variance_ = angle_variance;

        LOG(to_string(m.time) + "; angle variance = " + to_string(stddev_degrees(*angle_variance_)));
}

template <typename T>
bool PositionEstimation<T>::angle_variance_less_than(const T variance) const
{
        if (!angle_variance_)
        {
                return false;
        }

        for (std::size_t i = 0; i < 2; ++i)
        {
                if (!((*angle_variance_)[i] < variance))
                {
                        return false;
                }
        }

        return true;
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
Vector<2, T> PositionEstimation<T>::velocity() const
{
        return position_->velocity();
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

template class PositionEstimation<float>;
template class PositionEstimation<double>;
template class PositionEstimation<long double>;
}
