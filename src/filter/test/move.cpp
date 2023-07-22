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
Move<T>::Move(std::string name, const color::RGB8 color, const T reset_dt, std::unique_ptr<MoveFilter<T>>&& filter)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          filter_(std::move(filter))
{
        ASSERT(filter_);
}

template <typename T>
void Move<T>::save(const T time, const TrueData<2, T>& true_data)
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
void Move<T>::update(const Measurements<2, T>& m, const PositionEstimation<T>& position_estimation)
{
        check_time(m.time);

        if (!last_filter_time_ || !last_position_time_ || !(m.time - *last_filter_time_ < reset_dt_)
            || !(m.time - *last_position_time_ < reset_dt_))
        {
                if (position_estimation.has_estimates())
                {
                        LOG(name_ + "; " + position_estimation.description());

                        filter_->reset(
                                position_estimation.position_velocity_acceleration(),
                                position_estimation.position_velocity_acceleration_p(), position_estimation.angle());

                        last_filter_time_ = m.time;
                        last_position_time_ = m.time;
                }
                return;
        }

        const auto predict = [&]()
        {
                filter_->predict(m.time - *last_filter_time_);
        };

        if (m.position)
        {
                last_position_time_ = m.time;

                if (m.speed)
                {
                        if (m.direction)
                        {
                                predict();
                                filter_->update_position_speed_direction(*m.position, *m.speed, *m.direction);
                        }
                        else
                        {
                                predict();
                                filter_->update_position_speed(*m.position, *m.speed);
                        }
                }
                else
                {
                        if (m.direction)
                        {
                                predict();
                                filter_->update_position_direction(*m.position, *m.direction);
                        }
                        else
                        {
                                predict();
                                filter_->update_position(*m.position);
                        }
                }
        }
        else
        {
                if (m.speed)
                {
                        if (m.direction)
                        {
                                predict();
                                filter_->update_speed_direction(*m.speed, *m.direction);
                        }
                        else
                        {
                                predict();
                                filter_->update_speed(*m.speed);
                        }
                }
                else
                {
                        if (m.direction)
                        {
                                predict();
                                filter_->update_direction(*m.direction);
                        }
                        else
                        {
                                return;
                        }
                }
        }

        last_filter_time_ = m.time;

        save(m.time, m.true_data);

        if (m.position)
        {
                LOG(to_string(m.time) + "; true angle = "
                    + to_string(radians_to_degrees(normalize_angle(m.true_data.angle + m.true_data.angle_r))) + "; "
                    + angle_string());
        }
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
        s += name + "; NEES position; " + nees_->position.check_string();
        s += '\n';
        s += name + "; NEES speed; " + nees_->speed.check_string();
        s += '\n';
        s += name + "; NEES angle; " + nees_->angle.check_string();
        return s;
}

template <typename T>
const std::vector<Vector<3, T>>& Move<T>::positions() const
{
        return positions_;
}

template <typename T>
const std::vector<Vector<3, T>>& Move<T>::positions_p() const
{
        return positions_p_;
}

template <typename T>
const std::vector<Vector<2, T>>& Move<T>::speeds() const
{
        return speeds_;
}

template <typename T>
const std::vector<Vector<2, T>>& Move<T>::speeds_p() const
{
        return speeds_p_;
}

template class Move<float>;
template class Move<double>;
template class Move<long double>;
}
