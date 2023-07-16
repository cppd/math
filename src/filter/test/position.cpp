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
#include <src/com/type/name.h>

namespace ns::filter::test
{
namespace
{
constexpr std::size_t VARIANCE_WINDOW_SIZE = 500;
constexpr unsigned VARIANCE_SKIP_UPDATE_COUNT = 5;
}

template <typename T>
Position<T>::Position(
        std::string name,
        const color::RGB8 color,
        const T reset_dt,
        std::unique_ptr<PositionFilter<T>>&& filter)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          filter_(std::move(filter)),
          variance_(VARIANCE_WINDOW_SIZE)
{
        ASSERT(filter_);
}

template <typename T>
void Position<T>::save(const T time, const TrueData<2, T>& true_data)
{
        {
                const Vector<2, T> p = filter_->position();
                positions_.push_back({time, p[0], p[1]});
        }

        speeds_.push_back({time, filter_->speed()});
        speeds_p_.push_back({time, filter_->speed_p()});

        nees_position_.add(true_data.position - filter_->position(), filter_->position_p());

        if (const T speed_p = filter_->speed_p(); is_finite(speed_p))
        {
                nees_speed_.add(true_data.speed - filter_->speed(), speed_p);
        }
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

        if (!last_filter_time_ || !last_position_time_ || !(m.time - *last_position_time_ < reset_dt_))
        {
                filter_->reset(m.position->value, m.position->variance);
                last_filter_time_ = m.time;
                last_position_time_ = m.time;
                update_count_ = 0;
                return;
        }

        filter_->predict(m.time - *last_filter_time_);
        const PositionFilterUpdate update = filter_->update(m.position->value, m.position->variance);
        last_filter_time_ = m.time;
        last_position_time_ = m.time;

        ++update_count_;
        if (update_count_ > VARIANCE_SKIP_UPDATE_COUNT)
        {
                variance_.push(update.residual);
        }

        save(m.time, m.true_data);
}

template <typename T>
void Position<T>::predict_update(const Measurements<2, T>& m)
{
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

        save(m.time, m.true_data);
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
        if (variance_.has_variance())
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
                s += "; Mean " + to_string(variance_.mean());
                s += "; Standard Deviation " + to_string(variance_.standard_deviation());
        }
        if (const std::string check_string = filter_->check_string(); !check_string.empty())
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
                s += "; " + check_string;
        }
        return s;
}

template <typename T>
const std::vector<Vector<3, T>>& Position<T>::positions() const
{
        return positions_;
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
