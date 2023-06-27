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
template <typename T>
Position<T>::Position(std::string name, color::RGB8 color, std::unique_ptr<PositionFilter<T>>&& filter)
        : name_(std::move(name)),
          color_(color),
          filter_(std::move(filter))
{
        ASSERT(filter_);
}

template <typename T>
void Position<T>::save(const T time, const TrueData<2, T>& true_data)
{
        {
                const Vector<2, T> p = filter_->position();
                position_.push_back({time, p[0], p[1]});
        }

        speed_.push_back({time, filter_->speed()});
        speed_p_.push_back({time, filter_->speed_p()});

        nees_position_.add(true_data.position - filter_->position(), filter_->position_p());

        if (const T speed_p = filter_->speed_p(); is_finite(speed_p))
        {
                nees_speed_.add(true_data.speed - filter_->speed(), speed_p);
        }
}

template <typename T>
void Position<T>::update(const Measurement<2, T>& m)
{
        ASSERT(!last_time_ || *last_time_ < m.time);

        if (!m.position)
        {
                error("No position in measurement");
        }

        const T delta = last_time_ ? (m.time - *last_time_) : 0;
        last_time_ = m.time;

        filter_->predict(delta);
        filter_->update(*m.position, m.position_variance);

        save(m.time, m.true_data);
}

template <typename T>
T Position<T>::angle() const
{
        return filter_->angle();
}

template <typename T>
T Position<T>::angle_p() const
{
        return filter_->angle_p();
}

template <typename T>
Vector<2, T> Position<T>::position() const
{
        return filter_->position();
}

template <typename T>
Vector<2, T> Position<T>::velocity() const
{
        return filter_->velocity();
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
        s += name + "; NEES Position; " + nees_position_.check_string();
        s += '\n';
        s += name + "; NEES Speed; " + nees_speed_.check_string();
        s += '\n';
        s += name + "; NIS Position; " + filter_->nis_position_check_string();
        return s;
}

template <typename T>
const std::vector<Vector<3, T>>& Position<T>::positions() const
{
        return position_;
}

template <typename T>
const std::vector<Vector<2, T>>& Position<T>::speeds() const
{
        return speed_;
}

template <typename T>
const std::vector<Vector<2, T>>& Position<T>::speeds_p() const
{
        return speed_p_;
}

template class Position<float>;
template class Position<double>;
template class Position<long double>;
}
