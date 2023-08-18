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

#include "move.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/type/name.h>

namespace ns::filter::test
{
template <typename T>
Move<T>::Move(
        std::string name,
        const color::RGB8 color,
        const T reset_dt,
        const T angle_p,
        std::unique_ptr<MoveFilter<T>>&& filter)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          angle_p_(angle_p),
          filter_(std::move(filter))
{
        ASSERT(filter_);
}

template <typename T>
void Move<T>::save(const T time, const TrueData<2, T>& true_data)
{
        positions_.push_back({.time = time, .point = filter_->position()});
        positions_p_.push_back({.time = time, .point = filter_->position_p().diagonal()});
        speeds_.push_back({.time = time, .point = Vector<1, T>(filter_->speed())});
        speeds_p_.push_back({.time = time, .point = Vector<1, T>(filter_->speed_p())});

        if (!nees_)
        {
                nees_.emplace();
        }
        nees_->position.add(true_data.position - filter_->position(), filter_->position_p());
        nees_->speed.add(true_data.speed - filter_->speed(), filter_->speed_p());
        nees_->angle.add(normalize_angle(true_data.angle + true_data.angle_r - filter_->angle()), filter_->angle_p());
}

template <typename T>
void Move<T>::check_time(const T time) const
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

template <typename T>
void Move<T>::update_position(
        const Measurement<2, T>& position,
        const Measurements<2, T>& m,
        const T dt,
        const bool has_angle,
        const bool use_gate)
{
        ASSERT(m.position);

        if (m.speed)
        {
                if (has_angle && m.direction)
                {
                        filter_->predict(dt);
                        filter_->update_position_speed_direction(position, *m.speed, *m.direction, use_gate);
                }
                else
                {
                        filter_->predict(dt);
                        filter_->update_position_speed(position, *m.speed, use_gate);
                }
        }
        else
        {
                if (has_angle && m.direction)
                {
                        filter_->predict(dt);
                        filter_->update_position_direction(position, *m.direction, use_gate);
                }
                else
                {
                        filter_->predict(dt);
                        filter_->update_position(position, use_gate);
                }
        }
}

template <typename T>
bool Move<T>::update_non_position(const Measurements<2, T>& m, const T dt, const bool has_angle, const bool use_gate)
{
        ASSERT(!m.position);

        if (m.speed)
        {
                if (has_angle && m.direction)
                {
                        filter_->predict(dt);
                        filter_->update_speed_direction(*m.speed, *m.direction, use_gate);
                }
                else
                {
                        filter_->predict(dt);
                        filter_->update_speed(*m.speed, use_gate);
                }
        }
        else
        {
                if (has_angle && m.direction)
                {
                        filter_->predict(dt);
                        filter_->update_direction(*m.direction, use_gate);
                }
                else
                {
                        return false;
                }
        }

        return true;
}

template <typename T>
void Move<T>::update(const Measurements<2, T>& m, const Estimation<T>& estimation)
{
        check_time(m.time);

        if (!m.position)
        {
                return;
        }

        const bool has_angle = estimation.has_position() && (estimation.position_angle_p() <= angle_p_);

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                if (m.position && has_angle)
                {
                        LOG(name_ + "; " + estimation.position_description());

                        filter_->reset(
                                estimation.position_velocity_acceleration(),
                                estimation.position_velocity_acceleration_p());

                        last_time_ = m.time;
                }
                return;
        }

        if (!m.position && last_position_time_ && !(m.time - *last_position_time_ < reset_dt_))
        {
                return;
        }

        const T dt = m.time - *last_time_;
        const bool use_gate = true;

        if (m.position)
        {
                if (!m.position->variance)
                {
                        return;
                }

                const Measurement<2, T> position = {.value = m.position->value, .variance = *m.position->variance};

                update_position(position, m, dt, has_angle, use_gate);

                last_position_time_ = m.time;

                LOG(to_string(m.time) + "; true angle = "
                    + to_string(radians_to_degrees(normalize_angle(m.true_data.angle + m.true_data.angle_r))) + "; "
                    + angle_string());
        }
        else
        {
                if (!update_non_position(m, dt, has_angle, use_gate))
                {
                        return;
                }
        }

        last_time_ = m.time;

        save(m.time, m.true_data);
}

template <typename T>
const std::string& Move<T>::name() const
{
        return name_;
}

template <typename T>
color::RGB8 Move<T>::color() const
{
        return color_;
}

template <typename T>
std::string Move<T>::angle_string() const
{
        std::string s;
        s += name_;
        s += "; angle = " + to_string(radians_to_degrees(normalize_angle(filter_->angle())));
        s += "; angle speed = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_speed())));
        return s;
}

template <typename T>
std::string Move<T>::consistency_string() const
{
        if (!nees_)
        {
                return {};
        }

        const std::string name = std::string("Move<") + type_name<T>() + "> " + name_;

        std::string s;

        const auto new_line = [&]()
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
        };

        new_line();
        s += "; NEES position; " + nees_->position.check_string();

        new_line();
        s += "; NEES speed; " + nees_->speed.check_string();

        new_line();
        s += "; NEES angle; " + nees_->angle.check_string();

        return s;
}

template <typename T>
const std::vector<Point<2, T>>& Move<T>::positions() const
{
        return positions_;
}

template <typename T>
const std::vector<Point<2, T>>& Move<T>::positions_p() const
{
        return positions_p_;
}

template <typename T>
const std::vector<Point<1, T>>& Move<T>::speeds() const
{
        return speeds_;
}

template <typename T>
const std::vector<Point<1, T>>& Move<T>::speeds_p() const
{
        return speeds_p_;
}

template class Move<float>;
template class Move<double>;
template class Move<long double>;
}
