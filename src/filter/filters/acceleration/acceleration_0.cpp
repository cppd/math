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

#include "acceleration_0.h"

#include "init.h"
#include "update.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/filter/filters/estimation.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>

#include <cstddef>
#include <optional>
#include <string>

namespace ns::filter::filters::acceleration
{
template <typename T>
Acceleration0<T>::Acceleration0(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const T sigma_points_alpha,
        const T position_variance,
        const T angle_variance,
        const T angle_r_variance,
        const Init<T>& init)
        : reset_dt_(reset_dt),
          angle_estimation_variance_(angle_estimation_variance),
          gate_(gate),
          filter_(create_filter_0(sigma_points_alpha, position_variance, angle_variance, angle_r_variance)),
          init_(init),
          queue_(measurement_queue_size, reset_dt, angle_estimation_variance)
{
        ASSERT(filter_);
}

template <typename T>
void Acceleration0<T>::check_time(const T time) const
{
        if (last_time_ && !(*last_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_time_) + " to " + to_string(time));
        }
}

template <typename T>
std::optional<UpdateInfo<2, T>> Acceleration0<T>::update(
        const Measurements<2, T>& m,
        const Estimation<2, T>& estimation)
{
        check_time(m.time);

        queue_.update(m, estimation);

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                if (!m.position || queue_.empty())
                {
                        return {};
                }

                ASSERT(queue_.last_time() == m.time);

                queue_.update_filter(
                        [&]()
                        {
                                filter_->reset(
                                        queue_.init_position_velocity(), queue_.init_position_velocity_p(), init_);
                        },
                        [&](const Measurement<2, T>& position, const Measurements<2, T>& measurements, const T dt)
                        {
                                update_position(
                                        filter_.get(), position, measurements.acceleration, measurements.direction,
                                        measurements.speed, gate_, dt, nis_);
                        });

                last_time_ = m.time;
                return {};
        }

        const T dt = m.time - *last_time_;

        if (m.position)
        {
                if (!m.position->variance)
                {
                        return {};
                }

                const Measurement<2, T> position = {.value = m.position->value, .variance = *m.position->variance};

                update_position(filter_.get(), position, m.acceleration, m.direction, m.speed, gate_, dt, nis_);

                LOG(to_string(m.time) + "; true angle = " + to_string(radians_to_degrees(m.true_data.angle)) + "; "
                    + "; angle = " + to_string(radians_to_degrees(normalize_angle(filter_->angle())))
                    + "; angle r = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_r()))));
        }
        else
        {
                if (!update_non_position(filter_.get(), m.acceleration, m.direction, m.speed, gate_, dt, nis_))
                {
                        return {};
                }
        }

        last_time_ = m.time;

        update_nees(*filter_, m.true_data, nees_);

        return {
                {.position = filter_->position(),
                 .position_p = filter_->position_p().diagonal(),
                 .speed = filter_->speed(),
                 .speed_p = filter_->speed_p()}
        };
}

template <typename T>
std::string Acceleration0<T>::consistency_string() const
{
        std::string s;

        if (nees_)
        {
                s += "NEES position; " + nees_->position.check_string();
                s += '\n';
                s += "NEES speed; " + nees_->speed.check_string();
                s += '\n';
                s += "NEES angle; " + nees_->angle.check_string();
                s += '\n';
                s += "NEES angle r; " + nees_->angle_r.check_string();
        }

        if (nis_)
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += "NIS position; " + nis_->position.check_string();
        }

        return s;
}

#define TEMPLATE(T) template class Acceleration0<T>;

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
