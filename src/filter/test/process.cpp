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
Process<T>::Process(std::string name, const unsigned char color, std::unique_ptr<ProcessFilter<T>>&& filter, T dt)
        : name_(std::move(name)),
          color_(color),
          filter_(std::move(filter)),
          dt_(dt)
{
        ASSERT(filter_);
}

template <typename T>
void Process<T>::save(const std::size_t index, const SimulatorPoint<2, T>& point)
{
        position_.push_back(filter_->position());
        speed_.push_back({index, filter_->speed()});

        nees_position_.add(point.position - filter_->position(), filter_->position_p());
        nees_angle_.add(normalize_angle(point.angle - filter_->angle()), filter_->angle_p());
        nees_angle_r_.add(normalize_angle(point.angle_r - filter_->angle_r()), filter_->angle_r_p());
}

template <typename T>
void Process<T>::predict(const std::size_t index)
{
        ASSERT(!last_position_i_ || *last_position_i_ < index);

        const std::size_t delta = last_position_i_ ? (index - *last_position_i_) : 1;
        filter_->predict(delta * dt_);
}

template <typename T>
void Process<T>::update(
        const PositionMeasurement<2, T>& measurement,
        const T position_variance,
        const T speed_variance,
        const std::size_t index,
        const SimulatorPoint<2, T>& point)
{
        predict(measurement.index);

        if (measurement.speed)
        {
                filter_->update_position_speed(
                        measurement.position, *measurement.speed, position_variance, speed_variance);
        }
        else
        {
                filter_->update_position(measurement.position, position_variance);
        }

        save(index, point);
}

template <typename T>
void Process<T>::update(
        const PositionMeasurement<2, T>& position_measurement,
        const ProcessMeasurement<2, T>& process_measurement,
        const T position_variance,
        const T speed_variance,
        const T direction_variance,
        const T acceleration_variance,
        const std::size_t index,
        const SimulatorPoint<2, T>& point)
{
        predict(position_measurement.index);

        if (position_measurement.speed)
        {
                filter_->update_position_speed_direction_acceleration(
                        position_measurement.position, *position_measurement.speed, process_measurement.direction,
                        process_measurement.acceleration, position_variance, speed_variance, direction_variance,
                        acceleration_variance);
        }
        else
        {
                filter_->update_position_direction_acceleration(
                        position_measurement.position, process_measurement.direction, process_measurement.acceleration,
                        position_variance, direction_variance, acceleration_variance);
        }

        save(index, point);
}

template <typename T>
void Process<T>::update(
        const ProcessMeasurement<2, T>& measurement,
        const T acceleration_variance,
        const std::size_t index,
        const SimulatorPoint<2, T>& point)
{
        predict(measurement.index);

        filter_->update_acceleration(measurement.acceleration, acceleration_variance);

        save(index, point);
}

template <typename T>
const std::string& Process<T>::name() const
{
        return name_;
}

template <typename T>
unsigned char Process<T>::color() const
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
const std::vector<Vector<2, T>>& Process<T>::position() const
{
        return position_;
}
template <typename T>
const std::vector<Vector<2, T>>& Process<T>::speed() const
{
        return speed_;
}

template class Process<float>;
template class Process<double>;
template class Process<long double>;
}
