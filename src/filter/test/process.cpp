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

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/type/name.h>

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
void Process<T>::save(const T time, const TrueData<2, T>& true_data)
{
        const Vector<2, T> p = filter_->position();
        position_.push_back({time, p[0], p[1]});
        speed_.push_back({time, filter_->speed()});
        speed_p_.push_back({time, filter_->speed_p()});

        if (!nees_)
        {
                nees_.emplace();
        }
        nees_->position.add(true_data.position - filter_->position(), filter_->position_p());
        nees_->angle.add(normalize_angle(true_data.angle - filter_->angle()), filter_->angle_p());
        nees_->angle_r.add(normalize_angle(true_data.angle_r - filter_->angle_r()), filter_->angle_r_p());
}

template <typename T>
void Process<T>::update(const Measurement<2, T>& m, const PositionEstimation<T>& position_estimation)
{
        ASSERT(!last_time_ || *last_time_ < m.time);

        if (!last_time_)
        {
                if (position_estimation.has_estimates())
                {
                        filter_->reset(
                                position_estimation.position(), position_estimation.velocity(),
                                position_estimation.angle(), m.position_variance);
                        last_time_ = m.time;
                }
                return;
        }

        const T delta = m.time - *last_time_;
        last_time_ = m.time;

        filter_->predict(delta);

        if (m.position && m.speed && m.direction && m.acceleration)
        {
                filter_->update_position_speed_direction_acceleration(
                        *m.position, *m.speed, *m.direction, *m.acceleration, m.position_variance, m.speed_variance,
                        m.direction_variance, m.acceleration_variance);
        }
        else if (m.position && m.speed && !m.direction && !m.acceleration)
        {
                filter_->update_position_speed(*m.position, *m.speed, m.position_variance, m.speed_variance);
        }
        else if (m.position && !m.speed && m.direction && m.acceleration)
        {
                filter_->update_position_direction_acceleration(
                        *m.position, *m.direction, *m.acceleration, m.position_variance, m.direction_variance,
                        m.acceleration_variance);
        }
        else if (m.position && !m.speed && !m.direction && !m.acceleration)
        {
                filter_->update_position(*m.position, m.position_variance);
        }
        else if (!m.position && m.speed && !m.direction && m.acceleration)
        {
                filter_->update_speed_acceleration(
                        *m.speed, *m.acceleration, m.speed_variance, m.acceleration_variance);
        }
        else if (m.acceleration)
        {
                filter_->update_acceleration(*m.acceleration, m.acceleration_variance);
        }

        save(m.time, m.true_data);

        if (m.position)
        {
                LOG(to_string(m.time) + "; true angle = " + to_string(radians_to_degrees(m.true_data.angle)) + "; "
                    + angle_string());
        }
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
std::string Process<T>::angle_string() const
{
        std::string s;
        s += name_;
        s += "; process = " + to_string(radians_to_degrees(normalize_angle(filter_->angle())));
        s += "; speed = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_speed())));
        s += "; r = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_r())));
        return s;
}

template <typename T>
std::string Process<T>::consistency_string() const
{
        if (!nees_)
        {
                return {};
        }

        const std::string name = std::string("Process<") + type_name<T>() + "> " + name_;
        std::string s;
        s += name + "; NEES Position; " + nees_->position.check_string();
        s += '\n';
        s += name + "; NEES Angle; " + nees_->angle.check_string();
        s += '\n';
        s += name + "; NEES Angle R; " + nees_->angle_r.check_string();
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

template <typename T>
const std::vector<Vector<2, T>>& Process<T>::speeds_p() const
{
        return speed_p_;
}

template class Process<float>;
template class Process<double>;
template class Process<long double>;
}
