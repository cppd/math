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

#include "position_2.h"

#include "filter_2.h"
#include "init.h"

#include "../../consistency.h"
#include "../../settings/instantiation.h"
#include "../filter.h"
#include "../measurement.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::position
{
template <std::size_t N, typename T>
Position2<N, T>::Position2(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const T theta,
        const T process_variance,
        const Init<T>& init)
        : reset_dt_(reset_dt),
          linear_dt_(linear_dt),
          gate_(gate),
          filter_(create_filter_2<N, T>(theta, process_variance)),
          init_(init)
{
        ASSERT(filter_);
}

template <std::size_t N, typename T>
void Position2<N, T>::add_nees_checks(const TrueData<N, T>& true_data)
{
        nees_position_.add(true_data.position - filter_->position(), filter_->position_p());

        if (const T speed_p = filter_->speed_p(); is_finite(speed_p))
        {
                nees_speed_.add(true_data.speed - filter_->speed(), speed_p);
        }
}

template <std::size_t N, typename T>
void Position2<N, T>::check_time(const T time) const
{
        if (last_predict_time_ && !(*last_predict_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_predict_time_) + " to "
                      + to_string(time));
        }

        if (last_update_time_ && !(*last_update_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_update_time_) + " to "
                      + to_string(time));
        }
}

template <std::size_t N, typename T>
std::optional<UpdateInfo<N, T>> Position2<N, T>::update(const Measurements<N, T>& m)
{
        check_time(m.time);

        if (!m.position || !m.position->variance)
        {
                return {};
        }

        if (!last_predict_time_ || !last_update_time_ || !(m.time - *last_update_time_ < reset_dt_))
        {
                filter_->reset(m.position->value, *m.position->variance, init_);
                last_predict_time_ = m.time;
                last_update_time_ = m.time;
                add_nees_checks(m.true_data);
                return {
                        {.position = filter_->position(),
                         .position_p = filter_->position_p().diagonal(),
                         .speed = filter_->speed(),
                         .speed_p = filter_->speed_p()}
                };
        }

        filter_->predict(m.time - *last_predict_time_);
        last_predict_time_ = m.time;

        const auto update = filter_->update(m.position->value, *m.position->variance, gate_);
        if (update.gate)
        {
                add_nees_checks(m.true_data);
                return {
                        {.position = filter_->position(),
                         .position_p = filter_->position_p().diagonal(),
                         .speed = filter_->speed(),
                         .speed_p = filter_->speed_p()}
                };
        }
        const T update_dt = m.time - *last_update_time_;
        last_update_time_ = m.time;

        add_nees_checks(m.true_data);
        if (update_dt <= linear_dt_)
        {
                nis_.add(update.normalized_innovation_squared);
        }

        return {
                {.position = filter_->position(),
                 .position_p = filter_->position_p().diagonal(),
                 .speed = filter_->speed(),
                 .speed_p = filter_->speed_p()}
        };
}

template <std::size_t N, typename T>
std::optional<UpdateInfo<N, T>> Position2<N, T>::predict(const Measurements<N, T>& m)
{
        if (m.position)
        {
                error("Predict with position");
        }

        check_time(m.time);

        if (!last_predict_time_ || !last_update_time_ || !(m.time - *last_update_time_ < reset_dt_))
        {
                return {};
        }

        filter_->predict(m.time - *last_predict_time_);
        last_predict_time_ = m.time;

        add_nees_checks(m.true_data);

        return {
                {.position = filter_->position(),
                 .position_p = filter_->position_p().diagonal(),
                 .speed = filter_->speed(),
                 .speed_p = filter_->speed_p()}
        };
}

template <std::size_t N, typename T>
[[nodiscard]] bool Position2<N, T>::empty() const
{
        return !last_predict_time_ || !last_update_time_;
}

template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> Position2<N, T>::position() const
{
        return filter_->position();
}

template <std::size_t N, typename T>
[[nodiscard]] Matrix<N, N, T> Position2<N, T>::position_p() const
{
        return filter_->position_p();
}

template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> Position2<N, T>::velocity() const
{
        return filter_->velocity();
}

template <std::size_t N, typename T>
[[nodiscard]] Matrix<N, N, T> Position2<N, T>::velocity_p() const
{
        return filter_->velocity_p();
}

template <std::size_t N, typename T>
Vector<2 * N, T> Position2<N, T>::position_velocity() const
{
        return filter_->position_velocity();
}

template <std::size_t N, typename T>
Matrix<2 * N, 2 * N, T> Position2<N, T>::position_velocity_p() const
{
        return filter_->position_velocity_p();
}

template <std::size_t N, typename T>
Vector<3 * N, T> Position2<N, T>::position_velocity_acceleration() const
{
        return filter_->position_velocity_acceleration();
}

template <std::size_t N, typename T>
Matrix<3 * N, 3 * N, T> Position2<N, T>::position_velocity_acceleration_p() const
{
        return filter_->position_velocity_acceleration_p();
}

template <std::size_t N, typename T>
std::string Position2<N, T>::consistency_string() const
{
        std::string s;

        const auto new_line = [&]()
        {
                if (!s.empty())
                {
                        s += '\n';
                }
        };

        if (!nees_position_.empty())
        {
                new_line();
                s += "NEES Position; " + nees_position_.check_string();
        }

        if (!nees_speed_.empty())
        {
                new_line();
                s += "NEES Speed; " + nees_speed_.check_string();
        }

        if (!nis_.empty())
        {
                new_line();
                s += "NIS Position; " + nis_.check_string();
        }

        return s;
}

#define TEMPLATE(N, T) template class Position2<(N), T>;

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
