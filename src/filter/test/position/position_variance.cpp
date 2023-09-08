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

#include "position_variance.h"

#include "position_filter_lkf_2.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/name.h>

namespace ns::filter::test::position
{
namespace
{
template <std::size_t N, typename T>
constexpr Vector<N, T> VARIANCE{square(T{1})};

template <typename T>
constexpr std::optional<T> GATE{T{250}};

template <typename T>
constexpr T VARIANCE_GATE_SQUARED{square(T{10})};

template <typename T>
constexpr T THETA{0};

template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> correct_residual(const Vector<N, T>& residual, const T dt)
{
        return residual / (dt + 1);
}

template <std::size_t N, typename T>
[[nodiscard]] bool check_residual(const Vector<N, T>& residual, const std::optional<Vector<N, T>>& variance)
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
PositionVariance<N, T>::PositionVariance(
        std::string name,
        const color::RGB8 color,
        const T reset_dt,
        const T process_variance)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          filter_(create_position_filter_lkf_2<N, T>(THETA<T>, process_variance))
{
        ASSERT(filter_);
}

template <std::size_t N, typename T>
void PositionVariance<N, T>::save_results(const T time)
{
        positions_.push_back({.time = time, .point = filter_->position()});
        positions_p_.push_back({.time = time, .point = filter_->position_p().diagonal()});
        speeds_.push_back({.time = time, .point = Vector<1, T>(filter_->speed())});
        speeds_p_.push_back({.time = time, .point = Vector<1, T>(filter_->speed_p())});
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
        filter_->predict(predict_dt);
        last_predict_time_ = m.time;

        const auto update = filter_->update(m.position->value, VARIANCE<N, T>, GATE<T>);
        if (update.gate)
        {
                return;
        }
        last_update_time_ = m.time;

        const Vector<N, T> residual = correct_residual(update.residual, predict_dt);

        if (!check_residual(residual, position_variance_.variance()))
        {
                LOG(to_string(m.time) + "; " + name_ + "; Discarded Residual = " + to_string(update.residual));
                return;
        }

        position_variance_.push(residual);

        if (!position_variance_.has_variance())
        {
                ASSERT(!last_position_variance_);
                LOG(to_string(m.time) + "; " + name_ + "; Residual = " + to_string(update.residual));
                return;
        }

        const auto standard_deviation = position_variance_.standard_deviation();
        ASSERT(standard_deviation);
        LOG(to_string(m.time) + "; " + name_ + "; Standard Deviation = " + to_string(*standard_deviation));

        const auto new_variance = position_variance_.compute();
        ASSERT(new_variance);
        last_position_variance_ = *new_variance;
}

template <std::size_t N, typename T>
void PositionVariance<N, T>::update_position(const Measurements<N, T>& m)
{
        check_time(m.time);

        if (!m.position)
        {
                return;
        }

        if (!last_predict_time_ || !last_update_time_ || !(m.time - *last_update_time_ < reset_dt_))
        {
                filter_->reset(m.position->value, VARIANCE<N, T>);
                last_predict_time_ = m.time;
                last_update_time_ = m.time;
        }
        else
        {
                update_position_variance(m);
        }

        save_results(m.time);
}

template <std::size_t N, typename T>
const std::string& PositionVariance<N, T>::name() const
{
        return name_;
}

template <std::size_t N, typename T>
color::RGB8 PositionVariance<N, T>::color() const
{
        return color_;
}

template <std::size_t N, typename T>
const std::optional<Vector<N, T>>& PositionVariance<N, T>::last_position_variance() const
{
        return last_position_variance_;
}

template <std::size_t N, typename T>
std::string PositionVariance<N, T>::consistency_string() const
{
        const std::string name = std::string("Position<") + type_name<T>() + "> " + name_;

        std::string s;

        const auto new_line = [&]()
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
        };

        if (const auto& mean = position_variance_.mean())
        {
                new_line();
                s += "; Mean " + to_string(*mean);
        }

        if (const auto& standard_deviation = position_variance_.standard_deviation())
        {
                new_line();
                s += "; Standard Deviation " + to_string(*standard_deviation);
        }

        return s;
}

#define TEMPLATE_N_T(N, T) template class PositionVariance<(N), T>;

#define TEMPLATE_T(T) TEMPLATE_N_T(1, T) TEMPLATE_N_T(2, T) TEMPLATE_N_T(3, T)

TEMPLATE_T(float)
TEMPLATE_T(double)
TEMPLATE_T(long double)
}
