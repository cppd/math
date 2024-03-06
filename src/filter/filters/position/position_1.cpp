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

#include "position_1.h"

#include "filter_1.h"
#include "init.h"

#include <src/com/error.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::position
{
template <std::size_t N, typename T>
Position1<N, T>::Position1(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const T theta,
        const T process_variance,
        const Init<T>& init)
        : reset_dt_(reset_dt),
          linear_dt_(linear_dt),
          gate_(gate),
          filter_(create_filter_1<N, T>(theta, process_variance)),
          init_(init)
{
        ASSERT(filter_);
}

template <std::size_t N, typename T>
void Position1<N, T>::check_time(const T time) const
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
std::optional<UpdateInfo<N, T>> Position1<N, T>::update(const Measurements<N, T>& m)
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
                return {
                        {.position = filter_->position(),
                         .position_p = filter_->position_p().diagonal(),
                         .speed = filter_->speed(),
                         .speed_p = filter_->speed_p()}
                };
        }

        filter_->predict(m.time - *last_predict_time_);
        last_predict_time_ = m.time;

        update_nees(*filter_, m.true_data, nees_);

        const auto update = filter_->update(m.position->value, *m.position->variance, gate_);
        if (update.gate)
        {
                return {
                        {.position = filter_->position(),
                         .position_p = filter_->position_p().diagonal(),
                         .speed = filter_->speed(),
                         .speed_p = filter_->speed_p()}
                };
        }
        const T update_dt = m.time - *last_update_time_;
        last_update_time_ = m.time;

        if (update_dt <= linear_dt_)
        {
                update_nis(update, nis_);
        }

        return {
                {.position = filter_->position(),
                 .position_p = filter_->position_p().diagonal(),
                 .speed = filter_->speed(),
                 .speed_p = filter_->speed_p()}
        };
}

template <std::size_t N, typename T>
std::optional<UpdateInfo<N, T>> Position1<N, T>::predict(const Measurements<N, T>& m)
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

        update_nees(*filter_, m.true_data, nees_);

        return {
                {.position = filter_->position(),
                 .position_p = filter_->position_p().diagonal(),
                 .speed = filter_->speed(),
                 .speed_p = filter_->speed_p()}
        };
}

template <std::size_t N, typename T>
[[nodiscard]] bool Position1<N, T>::empty() const
{
        return !last_predict_time_ || !last_update_time_;
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, T> Position1<N, T>::position() const
{
        return filter_->position();
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<N, N, T> Position1<N, T>::position_p() const
{
        return filter_->position_p();
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, T> Position1<N, T>::velocity() const
{
        return filter_->velocity();
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<N, N, T> Position1<N, T>::velocity_p() const
{
        return filter_->velocity_p();
}

template <std::size_t N, typename T>
numerical::Vector<2 * N, T> Position1<N, T>::position_velocity() const
{
        return filter_->position_velocity();
}

template <std::size_t N, typename T>
numerical::Matrix<2 * N, 2 * N, T> Position1<N, T>::position_velocity_p() const
{
        return filter_->position_velocity_p();
}

template <std::size_t N, typename T>
std::string Position1<N, T>::consistency_string() const
{
        return make_consistency_string(nees_, nis_);
}

#define TEMPLATE(N, T) template class Position1<(N), T>;

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
