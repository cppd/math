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

#include "speed.h"

#include "filter_1.h"
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
template <std::size_t N, typename T, template <std::size_t, typename> typename F>
Speed<N, T, F>::Speed(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const T sigma_points_alpha,
        const T position_variance,
        const Init<T>& init)
        : reset_dt_(reset_dt),
          gate_(gate),
          init_(init),
          queue_(measurement_queue_size, reset_dt, angle_estimation_variance)
{
        static_assert(std::is_same_v<F<N, T>, Filter1<N, T>> || std::is_same_v<F<N, T>, Filter2<N, T>>);

        if constexpr (std::is_same_v<F<N, T>, Filter1<N, T>>)
        {
                filter_ = create_filter_1<N, T>(sigma_points_alpha, position_variance);
        }
        if constexpr (std::is_same_v<F<N, T>, Filter2<N, T>>)
        {
                filter_ = create_filter_2<N, T>(sigma_points_alpha, position_variance);
        }

        ASSERT(filter_);
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
void Speed<N, T, F>::check_time(const T time) const
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

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
void Speed<N, T, F>::reset(const Measurements<N, T>& m)
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
                        update_position(filter_.get(), position, measurements.speed, gate_, dt, nis_);
                });

        last_time_ = m.time;
        last_position_time_ = m.time;
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
std::optional<UpdateInfo<N, T>> Speed<N, T, F>::update(const Measurements<N, T>& m, const Estimation<N, T>& estimation)
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
                update_position(filter_.get(), position, m.speed, gate_, dt, nis_);

                last_position_time_ = m.time;
        }
        else
        {
                if (!update_non_position(filter_.get(), m.speed, gate_, dt, nis_))
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

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
std::string Speed<N, T, F>::consistency_string() const
{
        return make_consistency_string(nees_, nis_);
}

#define TEMPLATE(N, T)                         \
        template class Speed<(N), T, Filter1>; \
        template class Speed<(N), T, Filter2>;

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
