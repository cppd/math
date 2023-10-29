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

#include "../utility/utility.h"

#include <src/com/conversion.h>
#include <src/com/log.h>

namespace ns::filter::filters::estimation
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

template <std::size_t N, typename T>
PositionEstimation<N, T>::PositionEstimation(const position::Position2<N, T>* const position)
        : position_(position)
{
}

template <std::size_t N, typename T>
void PositionEstimation<N, T>::update(const Measurements<N, T>& m)
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

        const Vector<N, T> angle_variance =
                utility::compute_angle_variance(position_->velocity(), position_->velocity_p());
        if (!is_finite(angle_variance))
        {
                return;
        }

        angle_variance_ = angle_variance;

        LOG(to_string(m.time) + "; angle variance = " + to_string(stddev_degrees(*angle_variance_)));
}

template <std::size_t N, typename T>
bool PositionEstimation<N, T>::angle_variance_less_than(const T variance) const
{
        if (!angle_variance_)
        {
                return false;
        }

        for (std::size_t i = 0; i < N; ++i)
        {
                if (!((*angle_variance_)[i] < variance))
                {
                        return false;
                }
        }

        return true;
}

template <std::size_t N, typename T>
Vector<2 * N, T> PositionEstimation<N, T>::position_velocity() const
{
        return position_->position_velocity();
}

template <std::size_t N, typename T>
Matrix<2 * N, 2 * N, T> PositionEstimation<N, T>::position_velocity_p() const
{
        return position_->position_velocity_p();
}

#define TEMPLATE_N_T(N, T) template class PositionEstimation<(N), T>;

#define TEMPLATE_T(T) TEMPLATE_N_T(1, T) TEMPLATE_N_T(2, T) TEMPLATE_N_T(3, T)

TEMPLATE_T(float)
TEMPLATE_T(double)
TEMPLATE_T(long double)
}
