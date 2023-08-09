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

#include "position.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/type/name.h>

namespace ns::filter::test
{
template <std::size_t N, typename T>
Position<N, T>::Position(
        std::string name,
        const color::RGB8 color,
        const T reset_dt,
        const T linear_dt,
        std::unique_ptr<PositionFilter<N, T>>&& filter)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          linear_dt_(linear_dt),
          filter_(std::move(filter))
{
        ASSERT(filter_);
}

template <std::size_t N, typename T>
void Position<N, T>::save_results(const T time)
{
        positions_.push_back({.time = time, .point = filter_->position()});
        positions_p_.push_back({.time = time, .point = filter_->position_p().diagonal()});
        speeds_.push_back({.time = time, .point = Vector<1, T>(filter_->speed())});
        speeds_p_.push_back({.time = time, .point = Vector<1, T>(filter_->speed_p())});
}

template <std::size_t N, typename T>
void Position<N, T>::add_nees_checks(const TrueData<N, T>& true_data)
{
        nees_position_.add(true_data.position - filter_->position(), filter_->position_p());
        if (const T speed_p = filter_->speed_p(); is_finite(speed_p))
        {
                nees_speed_.add(true_data.speed - filter_->speed(), speed_p);
        }
}

template <std::size_t N, typename T>
void Position<N, T>::check_time(const T time) const
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
void Position<N, T>::check_position_variance(const PositionMeasurement<N, T>& m)
{
        if (use_measurement_variance_)
        {
                if (*use_measurement_variance_ != m.variance.has_value())
                {
                        error("Different variance modes are not supported");
                }
                return;
        }

        use_measurement_variance_ = m.variance.has_value();
        if (!*use_measurement_variance_)
        {
                position_variance_.emplace();
        }
}

template <std::size_t N, typename T>
bool Position<N, T>::prepare_position_variance(const Measurements<N, T>& m)
{
        ASSERT(m.position);
        ASSERT(last_predict_time_);
        ASSERT(last_update_time_);

        if (!position_variance_)
        {
                ASSERT(m.position->variance);
                last_position_variance_ = *m.position->variance;
                return true;
        }

        const T dt = m.time - *last_update_time_;
        if (!(dt <= linear_dt_))
        {
                error("Variance computations require dt " + to_string(dt) + " to be less than or equal to "
                      + to_string(linear_dt_));
        }

        ASSERT(!m.position->variance);
        if (last_position_variance_)
        {
                return true;
        }

        ASSERT(!position_variance_->has_variance());

        filter_->predict(m.time - *last_predict_time_);
        const auto update =
                filter_->update(m.position->value, position_variance_->default_variance(), /*use_gate=*/false);
        ASSERT(update);
        last_predict_time_ = m.time;
        last_update_time_ = m.time;

        position_variance_->push(update->residual);
        LOG(to_string(m.time) + "; " + name_ + "; Residual = " + to_string(update->residual));
        const auto new_variance = position_variance_->compute();
        if (!new_variance)
        {
                return false;
        }

        filter_->reset(m.position->value, *new_variance);
        last_position_variance_ = *new_variance;
        const auto standard_deviation = position_variance_->standard_deviation();
        ASSERT(standard_deviation);
        LOG(to_string(m.time) + "; " + name_ + "; Initial Standard Deviation = " + to_string(*standard_deviation));

        return false;
}

template <std::size_t N, typename T>
void Position<N, T>::update_position_variance(const Measurements<N, T>& m, const PositionFilterUpdate<N, T>& update)
{
        if (!position_variance_)
        {
                return;
        }

        position_variance_->push(update.residual);

        const auto standard_deviation = position_variance_->standard_deviation();
        ASSERT(standard_deviation);
        LOG(to_string(m.time) + "; " + name_ + "; Standard Deviation = " + to_string(*standard_deviation));

        const auto new_variance = position_variance_->compute();
        ASSERT(new_variance);
        last_position_variance_ = *new_variance;
}

template <std::size_t N, typename T>
void Position<N, T>::update_position(const Measurements<N, T>& m)
{
        check_time(m.time);

        if (!m.position)
        {
                return;
        }

        check_position_variance(*m.position);

        if (!last_predict_time_ || !last_update_time_ || !(m.time - *last_update_time_ < reset_dt_))
        {
                ASSERT(position_variance_.has_value() != m.position->variance.has_value());
                const auto variance =
                        !position_variance_
                                ? *m.position->variance
                                : (last_position_variance_ ? *last_position_variance_
                                                           : position_variance_->default_variance());
                filter_->reset(m.position->value, variance);
                last_predict_time_ = m.time;
                last_update_time_ = m.time;
                save_results(m.time);
                add_nees_checks(m.true_data);
                return;
        }

        if (!prepare_position_variance(m))
        {
                return;
        }

        ASSERT(last_position_variance_);

        filter_->predict(m.time - *last_predict_time_);
        last_predict_time_ = m.time;

        const auto update = filter_->update(m.position->value, *last_position_variance_, /*use_gate=*/true);
        if (!update)
        {
                save_results(m.time);
                add_nees_checks(m.true_data);
                return;
        }
        const T update_dt = m.time - *last_update_time_;
        last_update_time_ = m.time;

        update_position_variance(m, *update);

        save_results(m.time);
        add_nees_checks(m.true_data);
        if (update_dt <= linear_dt_)
        {
                nis_.add(update->residual, update->r);
        }
}

template <std::size_t N, typename T>
void Position<N, T>::predict_update(const Measurements<N, T>& m)
{
        if (!last_position_variance_)
        {
                error("Prediction without variance");
        }

        if (m.position)
        {
                update_position(m);
                return;
        }

        check_time(m.time);

        if (!last_predict_time_ || !last_update_time_ || !(m.time - *last_update_time_ < reset_dt_))
        {
                return;
        }

        filter_->predict(m.time - *last_predict_time_);
        last_predict_time_ = m.time;

        save_results(m.time);
        add_nees_checks(m.true_data);
}

template <std::size_t N, typename T>
const std::string& Position<N, T>::name() const
{
        return name_;
}

template <std::size_t N, typename T>
color::RGB8 Position<N, T>::color() const
{
        return color_;
}

template <std::size_t N, typename T>
const std::optional<Vector<N, T>>& Position<N, T>::last_position_variance() const
{
        return last_position_variance_;
}

template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> Position<N, T>::velocity() const
{
        return filter_->velocity();
}

template <std::size_t N, typename T>
[[nodiscard]] Matrix<N, N, T> Position<N, T>::velocity_p() const
{
        return filter_->velocity_p();
}

template <std::size_t N, typename T>
Vector<3 * N, T> Position<N, T>::position_velocity_acceleration() const
{
        return filter_->position_velocity_acceleration();
}

template <std::size_t N, typename T>
Matrix<3 * N, 3 * N, T> Position<N, T>::position_velocity_acceleration_p() const
{
        return filter_->position_velocity_acceleration_p();
}

template <std::size_t N, typename T>
std::string Position<N, T>::consistency_string() const
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

        if (!nees_position_.empty())
        {
                new_line();
                s += "; NEES Position; " + nees_position_.check_string();
        }

        if (!nees_speed_.empty())
        {
                new_line();
                s += "; NEES Speed; " + nees_speed_.check_string();
        }

        if (!nis_.empty())
        {
                new_line();
                s += "; NIS Position; " + nis_.check_string();
        }

        if (position_variance_)
        {
                if (const auto& mean = position_variance_->mean())
                {
                        new_line();
                        s += "; Mean " + to_string(*mean);
                }

                if (const auto& standard_deviation = position_variance_->standard_deviation())
                {
                        new_line();
                        s += "; Standard Deviation " + to_string(*standard_deviation);
                }
        }

        return s;
}

template <std::size_t N, typename T>
const std::vector<Point<N, T>>& Position<N, T>::positions() const
{
        return positions_;
}

template <std::size_t N, typename T>
const std::vector<Point<N, T>>& Position<N, T>::positions_p() const
{
        return positions_p_;
}

template <std::size_t N, typename T>
const std::vector<Point<1, T>>& Position<N, T>::speeds() const
{
        return speeds_;
}

template <std::size_t N, typename T>
const std::vector<Point<1, T>>& Position<N, T>::speeds_p() const
{
        return speeds_p_;
}

#define TEMPLATE_N_T(N, T) template class Position<(N), T>;

#define TEMPLATE_T(T) TEMPLATE_N_T(1, T) TEMPLATE_N_T(2, T) TEMPLATE_N_T(3, T)

TEMPLATE_T(float)
TEMPLATE_T(double)
TEMPLATE_T(long double)
}
