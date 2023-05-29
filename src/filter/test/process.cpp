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

#include "process.h"

#include "utility.h"

#include <src/com/conversion.h>
#include <src/com/error.h>

namespace ns::filter::test
{
template <typename T>
Process<T>::Process(std::string name, const color::RGB8 color, std::unique_ptr<ProcessFilter<T>>&& filter)
        : name_(std::move(name)),
          color_(color),
          filter_(std::move(filter))
{
        ASSERT(filter_);
}

template <typename T>
void Process<T>::save(const T time, const SimulatorPoint<2, T>& point)
{
        const Vector<2, T> p = filter_->position();
        position_.push_back({time, p[0], p[1]});
        speed_.push_back({time, filter_->speed()});

        nees_position_.add(point.position - filter_->position(), filter_->position_p());
        nees_angle_.add(normalize_angle(point.angle - filter_->angle()), filter_->angle_p());
        nees_angle_r_.add(normalize_angle(point.angle_r - filter_->angle_r()), filter_->angle_r_p());
}

template <typename T>
void Process<T>::predict(const T time)
{
        ASSERT(!last_time_ || *last_time_ < time);

        const T delta = last_time_ ? (time - *last_time_) : 0;
        last_time_ = time;
        filter_->predict(delta);
}

template <typename T>
void Process<T>::update(
        const ProcessMeasurement<2, T>& m,
        const T position_variance,
        const T speed_variance,
        const T direction_variance,
        const T acceleration_variance,
        const SimulatorPoint<2, T>& point)
{
        predict(m.time);

        if (m.position && m.speed && m.direction && m.acceleration)
        {
                filter_->update_position_speed_direction_acceleration(
                        *m.position, *m.speed, *m.direction, *m.acceleration, position_variance, speed_variance,
                        direction_variance, acceleration_variance);
        }
        else if (m.position && m.speed && !m.direction && !m.acceleration)
        {
                filter_->update_position_speed(*m.position, *m.speed, position_variance, speed_variance);
        }
        else if (m.position && !m.speed && m.direction && m.acceleration)
        {
                filter_->update_position_direction_acceleration(
                        *m.position, *m.direction, *m.acceleration, position_variance, direction_variance,
                        acceleration_variance);
        }
        else if (m.position && !m.speed && !m.direction && !m.acceleration)
        {
                filter_->update_position(*m.position, position_variance);
        }
        else if (!m.position && m.speed && !m.direction && m.acceleration)
        {
                filter_->update_speed_acceleration(*m.speed, *m.acceleration, speed_variance, acceleration_variance);
        }
        else if (m.acceleration)
        {
                filter_->update_acceleration(*m.acceleration, acceleration_variance);
        }

        save(m.time, point);
}

template <typename T>
const std::string& Process<T>::name() const
{
        return name_;
}

template <typename T>
color::RGB8 Process<T>::color() const
{
        return color_;
}

template <typename T>
std::string Process<T>::angle_string(const SimulatorPoint<2, T>& point) const
{
        std::string s;
        s += name_;
        s += "; track = " + to_string(radians_to_degrees(normalize_angle(point.angle)));
        s += "; process = " + to_string(radians_to_degrees(normalize_angle(filter_->angle())));
        s += "; speed = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_speed())));
        s += "; r = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_r())));
        return s;
}

template <typename T>
std::string Process<T>::nees_string() const
{
        std::string s;
        s += "Process " + name_ + " Position: " + nees_position_.check_string();
        s += '\n';
        s += "Process " + name_ + " Angle: " + nees_angle_.check_string();
        s += '\n';
        s += "Process " + name_ + " Angle R: " + nees_angle_r_.check_string();
        return s;
}
template <typename T>
const std::vector<Vector<3, T>>& Process<T>::positions() const
{
        return position_;
}
template <typename T>
const std::vector<Vector<2, T>>& Process<T>::speeds() const
{
        return speed_;
}

template class Process<float>;
template class Process<double>;
template class Process<long double>;
}
