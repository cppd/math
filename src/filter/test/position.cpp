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
        std::unique_ptr<PositionFilter<T>>&& filter)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
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
void Position<T>::add_checks(const TrueData<2, T>& true_data)
{
        nees_position_.add(true_data.position - filter_->position(), filter_->position_p());
        if (const T speed_p = filter_->speed_p(); is_finite(speed_p))
        {
                nees_speed_.add(true_data.speed - filter_->speed(), speed_p);
        }
}

template <typename T>
void Position<T>::add_checks(const PositionFilterUpdate<T>& update_data)
{
        nis_.add(update_data.residual, update_data.r);
}

template <typename T>
void Position<T>::check_time(const T time) const
{
        if (last_filter_time_ && !(*last_filter_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_filter_time_) + " to "
                      + to_string(time));
        }

        if (last_position_time_ && !(*last_position_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_position_time_) + " to "
                      + to_string(time));
        }
}

template <typename T>
void Position<T>::update_position(const Measurements<2, T>& m)
{
        check_time(m.time);

        if (!m.position)
        {
                return;
        }

        const Vector<2, T> measurement_variance =
                last_measurement_variance_ ? *last_measurement_variance_ : measurement_variance_.default_variance();

        if (!last_filter_time_ || !last_position_time_ || !(m.time - *last_position_time_ < reset_dt_))
        {
                filter_->reset(m.position->value, measurement_variance);
                last_filter_time_ = m.time;
                last_position_time_ = m.time;
                return;
        }

        filter_->predict(m.time - *last_filter_time_);

        const auto update = filter_->update(
                m.position->value, measurement_variance, /*use_gate=*/last_measurement_variance_.has_value());

        if (!update)
        {
                ASSERT(last_measurement_variance_);
                last_filter_time_ = m.time;
                save_results(m.time);
                add_checks(m.true_data);
                return;
        }

        last_filter_time_ = m.time;
        last_position_time_ = m.time;

        measurement_variance_.push(update->residual);

        if (const auto& standard_deviation = measurement_variance_.standard_deviation())
        {
                LOG(to_string(m.time) + "; " + name_ + "; Standard Deviation " + to_string(*standard_deviation));
        }
        else
        {
                LOG(to_string(m.time) + "; " + name_ + "; Residual " + to_string(update->residual));
        }

        const auto new_variance = measurement_variance_.compute();
        if (new_variance && !last_measurement_variance_)
        {
                filter_->reset(m.position->value, *new_variance);
                last_measurement_variance_ = new_variance;
                return;
        }
        last_measurement_variance_ = new_variance;

        if (last_measurement_variance_)
        {
                save_results(m.time);
                add_checks(m.true_data);
                add_checks(*update);
        }
}

template <typename T>
void Position<T>::predict_update(const Measurements<2, T>& m)
{
        if (!last_measurement_variance_)
        {
                error("Prediction without variance");
        }

        if (m.position)
        {
                update_position(m);
                return;
        }

        check_time(m.time);

        if (!last_filter_time_ || !last_position_time_ || !(m.time - *last_position_time_ < reset_dt_))
        {
                return;
        }

        filter_->predict(m.time - *last_filter_time_);
        last_filter_time_ = m.time;

        save_results(m.time);
        add_checks(m.true_data);
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
const std::optional<Vector<2, T>>& Position<T>::last_measurement_variance() const
{
        return last_measurement_variance_;
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
        if (const auto& mean = measurement_variance_.mean())
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
                s += "; Mean " + to_string(*mean);
        }
        if (const auto& standard_deviation = measurement_variance_.standard_deviation())
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
                s += "; Standard Deviation " + to_string(*standard_deviation);
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
