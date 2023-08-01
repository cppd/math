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
template <typename T>
Position<T>::Position(
        std::string name,
        const color::RGB8 color,
        const T reset_dt,
        const T linear_dt,
        std::unique_ptr<PositionFilter<T>>&& filter)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          linear_dt_(linear_dt),
          filter_(std::move(filter))
{
        ASSERT(filter_);
}

template <typename T>
void Position<T>::save_results(const T time)
{
        {
                const Vector<2, T> p = filter_->position();
                positions_.push_back({time, p[0], p[1]});
        }
        {
                const Matrix<2, 2, T> p = filter_->position_p();
                positions_p_.push_back({time, p(0, 0), p(1, 1)});
        }
        speeds_.push_back({time, filter_->speed()});
        speeds_p_.push_back({time, filter_->speed_p()});
}

template <typename T>
void Position<T>::add_nees_checks(const TrueData<2, T>& true_data)
{
        nees_position_.add(true_data.position - filter_->position(), filter_->position_p());
        if (const T speed_p = filter_->speed_p(); is_finite(speed_p))
        {
                nees_speed_.add(true_data.speed - filter_->speed(), speed_p);
        }
}

template <typename T>
void Position<T>::check_time(const T time) const
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

template <typename T>
void Position<T>::check_position_variance(const PositionMeasurement<2, T>& m)
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

template <typename T>
bool Position<T>::prepare_position_variance(const Measurements<2, T>& m)
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

template <typename T>
void Position<T>::update_position_variance(const Measurements<2, T>& m, const PositionFilterUpdate<T>& update)
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

template <typename T>
void Position<T>::update_position(const Measurements<2, T>& m)
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

template <typename T>
void Position<T>::predict_update(const Measurements<2, T>& m)
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

template <typename T>
const PositionFilter<T>* Position<T>::filter() const
{
        return filter_.get();
}

template <typename T>
const std::string& Position<T>::name() const
{
        return name_;
}

template <typename T>
color::RGB8 Position<T>::color() const
{
        return color_;
}

template <typename T>
const std::optional<Vector<2, T>>& Position<T>::last_position_variance() const
{
        return last_position_variance_;
}

template <typename T>
std::string Position<T>::consistency_string() const
{
        const std::string name = std::string("Position<") + type_name<T>() + "> " + name_;
        std::string s;
        if (!nees_position_.empty())
        {
                s += name;
                s += "; NEES Position; " + nees_position_.check_string();
        }
        if (!nees_speed_.empty())
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
                s += "; NEES Speed; " + nees_speed_.check_string();
        }
        if (!nis_.empty())
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
                s += "; NIS Position; " + nis_.check_string();
        }
        if (position_variance_)
        {
                if (const auto& mean = position_variance_->mean())
                {
                        if (!s.empty())
                        {
                                s += '\n';
                        }
                        s += name;
                        s += "; Mean " + to_string(*mean);
                }
                if (const auto& standard_deviation = position_variance_->standard_deviation())
                {
                        if (!s.empty())
                        {
                                s += '\n';
                        }
                        s += name;
                        s += "; Standard Deviation " + to_string(*standard_deviation);
                }
        }
        return s;
}

template <typename T>
const std::vector<Vector<3, T>>& Position<T>::positions() const
{
        return positions_;
}

template <typename T>
const std::vector<Vector<3, T>>& Position<T>::positions_p() const
{
        return positions_p_;
}

template <typename T>
const std::vector<Vector<2, T>>& Position<T>::speeds() const
{
        return speeds_;
}

template <typename T>
const std::vector<Vector<2, T>>& Position<T>::speeds_p() const
{
        return speeds_p_;
}

template class Position<float>;
template class Position<double>;
template class Position<long double>;
}
