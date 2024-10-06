/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/conversion.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::filter::filters::estimation
{
namespace
{
template <std::size_t N, typename T>
numerical::Vector<N, T> stddev_degrees(const numerical::Vector<N, T>& v)
{
        numerical::Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = radians_to_degrees(std::sqrt(v[i]));
        }
        return res;
}
}

template <std::size_t N, typename T>
PositionEstimation<N, T>::PositionEstimation(const FilterPosition<N, T>* const position)
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

        const numerical::Vector<N, T> angle_variance =
                com::compute_angle_variance(position_->velocity(), position_->velocity_p());
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
numerical::Vector<N, T> PositionEstimation<N, T>::velocity() const
{
        return position_->velocity();
}

template <std::size_t N, typename T>
numerical::Vector<2 * N, T> PositionEstimation<N, T>::position_velocity() const
{
        return position_->position_velocity();
}

template <std::size_t N, typename T>
numerical::Matrix<2 * N, 2 * N, T> PositionEstimation<N, T>::position_velocity_p() const
{
        return position_->position_velocity_p();
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, T> PositionEstimation<N, T>::position() const
{
        return position_->position();
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<N, N, T> PositionEstimation<N, T>::position_p() const
{
        return position_->position_p();
}

template <std::size_t N, typename T>
[[nodiscard]] T PositionEstimation<N, T>::speed() const
{
        return position_->speed();
}

template <std::size_t N, typename T>
[[nodiscard]] T PositionEstimation<N, T>::speed_p() const
{
        return position_->speed_p();
}

#define TEMPLATE(N, T) template class PositionEstimation<(N), T>;

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
