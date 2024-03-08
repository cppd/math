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

#include "position_variance.h"

#include "moving_variance.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/position/filter_2.h>
#include <src/filter/filters/position/init.h>
#include <src/filter/utility/instantiation.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::estimation
{
namespace
{
template <std::size_t N, typename T>
constexpr numerical::Vector<N, T> VARIANCE{square(T{1})};

template <typename T>
constexpr std::optional<T> GATE{T{250}};

template <typename T>
constexpr T VARIANCE_GATE_SQUARED{square(T{10})};

template <typename T>
constexpr T THETA{0};

template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, T> correct_residual(const numerical::Vector<N, T>& residual, const T dt)
{
        return residual / (dt + 1);
}

template <std::size_t N, typename T>
[[nodiscard]] bool check_residual(
        const numerical::Vector<N, T>& residual,
        const std::optional<numerical::Vector<N, T>>& variance)
{
        if (!variance)
        {
                return true;
        }

        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(square(residual[i]) <= (*variance)[i] * VARIANCE_GATE_SQUARED<T>))
                {
                        return false;
                }
        }

        return true;
}
}

template <std::size_t N, typename T>
PositionVariance<N, T>::PositionVariance(const T reset_dt, const T process_variance, const position::Init<T>& init)
        : reset_dt_(reset_dt),
          init_(init),
          process_variance_(process_variance),
          filter_(position::create_filter_2<N, T>(THETA<T>))
{
        ASSERT(filter_);
}

template <std::size_t N, typename T>
void PositionVariance<N, T>::check_time(const T time) const
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
void PositionVariance<N, T>::update_position_variance(const Measurements<N, T>& m)
{
        ASSERT(m.position);
        ASSERT(last_predict_time_);
        ASSERT(last_update_time_);

        const T predict_dt = m.time - *last_predict_time_;
        filter_->predict(predict_dt, process_variance_);
        last_predict_time_ = m.time;

        const auto update = filter_->update(m.position->value, VARIANCE<N, T>, GATE<T>);
        if (update.gate)
        {
                return;
        }
        last_update_time_ = m.time;

        const numerical::Vector<N, T> residual = correct_residual(update.residual, predict_dt);

        if (!check_residual(residual, position_variance_.variance()))
        {
                LOG(to_string(m.time) + "; Discarded Residual = " + to_string(update.residual));
                return;
        }

        position_variance_.push(residual);

        if (!position_variance_.has_variance())
        {
                ASSERT(!last_position_variance_);
                LOG(to_string(m.time) + "; Residual = " + to_string(update.residual));
                return;
        }

        const auto standard_deviation = position_variance_.standard_deviation();
        ASSERT(standard_deviation);
        LOG(to_string(m.time) + "; Standard Deviation = " + to_string(*standard_deviation));

        const auto new_variance = position_variance_.compute();
        ASSERT(new_variance);
        last_position_variance_ = *new_variance;
}

template <std::size_t N, typename T>
std::optional<UpdateInfo<N, T>> PositionVariance<N, T>::update(const Measurements<N, T>& m)
{
        check_time(m.time);

        if (!m.position)
        {
                return {};
        }

        if (!last_predict_time_ || !last_update_time_ || !(m.time - *last_update_time_ < reset_dt_))
        {
                filter_->reset(m.position->value, VARIANCE<N, T>, init_);
                last_predict_time_ = m.time;
                last_update_time_ = m.time;
        }
        else
        {
                update_position_variance(m);
        }

        return {
                {.position = filter_->position(),
                 .position_p = filter_->position_p().diagonal(),
                 .speed = filter_->speed(),
                 .speed_p = filter_->speed_p()}
        };
}

template <std::size_t N, typename T>
std::optional<UpdateInfo<N, T>> PositionVariance<N, T>::predict(const Measurements<N, T>& /*m*/)
{
        error("predict is not supported");
}

template <std::size_t N, typename T>
const std::optional<numerical::Vector<N, T>>& PositionVariance<N, T>::last_position_variance() const
{
        return last_position_variance_;
}

template <std::size_t N, typename T>
std::string PositionVariance<N, T>::consistency_string() const
{
        std::string s;

        const auto new_line = [&]()
        {
                if (!s.empty())
                {
                        s += '\n';
                }
        };

        if (const auto& mean = position_variance_.mean())
        {
                new_line();
                s += "Mean " + to_string(*mean);
        }

        if (const auto& standard_deviation = position_variance_.standard_deviation())
        {
                new_line();
                s += "Standard Deviation " + to_string(*standard_deviation);
        }

        return s;
}

#define TEMPLATE(N, T) template class PositionVariance<(N), T>;

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
