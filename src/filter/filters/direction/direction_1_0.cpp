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

#include "direction_1_0.h"

#include "filter_1_0.h"
#include "init.h"
#include "update.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/filter/filters/com/measurement_queue.h>
#include <src/filter/filters/estimation.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::direction
{
template <typename T>
Direction10<T>::Direction10(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const T sigma_points_alpha,
        const T position_variance,
        const T angle_variance,
        const Init<T>& init)
        : reset_dt_(reset_dt),
          angle_estimation_variance_(angle_estimation_variance),
          gate_(gate),
          filter_(create_filter_1_0(sigma_points_alpha, position_variance, angle_variance)),
          init_(init),
          queue_(measurement_queue_size, reset_dt, angle_estimation_variance)
{
        ASSERT(filter_);
}

template <typename T>
void Direction10<T>::check_time(const T time) const
{
        if (last_time_ && !(*last_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_time_) + " to " + to_string(time));
        }

        if (last_position_time_ && !(*last_position_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_position_time_) + " to "
                      + to_string(time));
        }
}

template <typename T>
void Direction10<T>::reset(const Measurements<2, T>& m)
{
        if (!m.position || queue_.empty())
        {
                return;
        }

        ASSERT(queue_.last_time() == m.time);

        queue_.update_filter(
                [&]()
                {
                        filter_->reset(queue_.init_position_velocity(), queue_.init_position_velocity_p(), init_);
                },
                [&](const Measurement<2, T>& position, const Measurements<2, T>& measurements, const T dt)
                {
                        update_position(
                                filter_.get(), position, measurements.direction, measurements.speed, gate_, dt, nis_);
                });

        last_time_ = m.time;
        last_position_time_ = m.time;
}

template <typename T>
std::optional<UpdateInfo<2, T>> Direction10<T>::update(const Measurements<2, T>& m, const Estimation<2, T>& estimation)
{
        check_time(m.time);

        queue_.update(m, estimation);

        if (!m.position)
        {
                return {};
        }

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                reset(m);
                return {};
        }

        if (!m.position && last_position_time_ && !(m.time - *last_position_time_ < reset_dt_))
        {
                return {};
        }

        const T dt = m.time - *last_time_;
        const bool has_angle = estimation.angle_variance_less_than(angle_estimation_variance_);

        if (m.position)
        {
                if (!m.position->variance)
                {
                        return {};
                }

                const Measurement<2, T> position = {.value = m.position->value, .variance = *m.position->variance};
                const std::optional<Measurement<1, T>> direction = has_angle ? m.direction : std::nullopt;
                update_position(filter_.get(), position, direction, m.speed, gate_, dt, nis_);

                last_position_time_ = m.time;

                LOG(to_string(m.time) + "; true angle = "
                    + to_string(radians_to_degrees(normalize_angle(m.true_data.angle + m.true_data.angle_r))) + "; "
                    + "; angle = " + to_string(radians_to_degrees(normalize_angle(filter_->angle()))));
        }
        else
        {
                const std::optional<Measurement<1, T>> direction = has_angle ? m.direction : std::nullopt;
                if (!update_non_position(filter_.get(), direction, m.speed, gate_, dt, nis_))
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
std::string Direction10<T>::consistency_string() const
{
        std::string s;

        if (nees_)
        {
                s += "NEES position; " + nees_->position.check_string();
                s += '\n';
                s += "NEES speed; " + nees_->speed.check_string();
                s += '\n';
                s += "NEES angle; " + nees_->angle.check_string();
        }

        if (nis_)
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += "NIS position; " + nis_->position.check_string();
                s += '\n';
                s += "NIS position SD; " + nis_->position_speed_direction.check_string();
        }

        return s;
}

#define TEMPLATE(T) template class Direction10<T>;

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
