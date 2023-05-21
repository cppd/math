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

#include "process_data.h"

#include "utility.h"

#include <src/com/conversion.h>
#include <src/com/error.h>

namespace ns::filter::test
{

template <typename T>
ProcessData<T>::ProcessData(std::string name, const unsigned char color, const ProcessFilter<T>* const filter)
        : name_(std::move(name)),
          color_(color),
          filter_(filter)
{
        ASSERT(filter_);
}

template <typename T>
void ProcessData<T>::save(const std::size_t index, const SimulatorPoint<2, T>& point)
{
        position_.push_back(filter_->position());
        speed_.push_back({index, filter_->speed()});

        nees_position_.add(point.position - filter_->position(), filter_->position_p());
        nees_angle_.add(normalize_angle(point.angle - filter_->angle()), filter_->angle_p());
        nees_angle_r_.add(normalize_angle(point.angle_r - filter_->angle_r()), filter_->angle_r_p());
}

template <typename T>
const std::string& ProcessData<T>::name() const
{
        return name_;
}

template <typename T>
unsigned char ProcessData<T>::color() const
{
        return color_;
}

template <typename T>
std::string ProcessData<T>::angle_string(const SimulatorPoint<2, T>& point) const
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
std::string ProcessData<T>::nees_string() const
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
const std::vector<Vector<2, T>>& ProcessData<T>::position() const
{
        return position_;
}
template <typename T>
const std::vector<Vector<2, T>>& ProcessData<T>::speed() const
{
        return speed_;
}

template class ProcessData<float>;
template class ProcessData<double>;
template class ProcessData<long double>;
}
