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

#include "speed_2.h"

#include "filter_2.h"
#include "init.h"
#include "update.h"

#include <src/com/error.h>
#include <src/filter/filters/com/measurement_queue.h>
#include <src/filter/filters/estimation.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::speed
{
template <std::size_t N, typename T>
Speed2<N, T>::Speed2(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const T sigma_points_alpha,
        const T position_variance,
        const Init<T>& init)
        : reset_dt_(reset_dt),
          gate_(gate),
          filter_(create_filter_2<N, T>(sigma_points_alpha, position_variance)),
          init_(init),
          queue_(measurement_queue_size, reset_dt, angle_estimation_variance)
{
        ASSERT(filter_);
}

template <std::size_t N, typename T>
void Speed2<N, T>::save(const TrueData<N, T>& true_data)
{
        if (!nees_)
        {
                nees_.emplace();
        }
        nees_->position.add(true_data.position - filter_->position(), filter_->position_p());
        nees_->speed.add(true_data.speed - filter_->speed(), filter_->speed_p());
}

template <std::size_t N, typename T>
void Speed2<N, T>::check_time(const T time) const
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

template <std::size_t N, typename T>
void Speed2<N, T>::reset(const Measurements<N, T>& m)
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
                [&](const Measurement<N, T>& position, const Measurements<N, T>& measurements, const T dt)
                {
                        update_position(filter_.get(), position, measurements.speed, gate_, dt);
                });

        last_time_ = m.time;
        last_position_time_ = m.time;
}

template <std::size_t N, typename T>
std::optional<UpdateInfo<N, T>> Speed2<N, T>::update(const Measurements<N, T>& m, const Estimation<N, T>& estimation)
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

        if (m.position)
        {
                if (!m.position->variance)
                {
                        return {};
                }

                const Measurement<N, T> position = {.value = m.position->value, .variance = *m.position->variance};
                update_position(filter_.get(), position, m.speed, gate_, dt);

                last_position_time_ = m.time;
        }
        else
        {
                if (!update_non_position(filter_.get(), m.speed, gate_, dt))
                {
                        return {};
                }
        }

        last_time_ = m.time;

        save(m.true_data);

        return {
                {.position = filter_->position(),
                 .position_p = filter_->position_p().diagonal(),
                 .speed = filter_->speed(),
                 .speed_p = filter_->speed_p()}
        };
}

template <std::size_t N, typename T>
std::string Speed2<N, T>::consistency_string() const
{
        if (!nees_)
        {
                return {};
        }

        std::string s;

        s += "NEES position; " + nees_->position.check_string();
        s += '\n';
        s += "NEES speed; " + nees_->speed.check_string();

        return s;
}

#define TEMPLATE(N, T) template class Speed2<(N), T>;

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
