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

#pragma once

#include "simulator.h"
#include "utility.h"

#include "../nees.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test
{
template <typename T, template <typename> typename Filter>
class ProcessFilterData final
{
        std::string name_;
        const Filter<T>* filter_;

        std::vector<Vector<2, T>> position_;
        std::vector<std::optional<T>> speed_;

        NeesAverage<2, T> nees_position_;
        NeesAverage<1, T> nees_angle_;
        NeesAverage<1, T> nees_angle_r_;

public:
        ProcessFilterData(
                std::string name,
                const Filter<T>* const filter,
                const std::size_t reserve,
                const std::size_t resize)
                : name_(std::move(name)),
                  filter_(filter)
        {
                ASSERT(filter_);

                position_.reserve(reserve);
                speed_.reserve(reserve);
                speed_.resize(resize);
        }

        void save(const SimulatorPoint<2, T>& point)
        {
                position_.push_back(filter_->position());
                speed_.push_back(filter_->speed());

                nees_position_.add(point.position - filter_->position(), filter_->position_p());
                nees_angle_.add(normalize_angle(point.angle - filter_->angle()), filter_->angle_p());
                nees_angle_r_.add(normalize_angle(point.angle_r - filter_->angle_r()), filter_->angle_r_p());
        }

        [[nodiscard]] std::string angle_string(const SimulatorPoint<2, T>& point) const
        {
                std::string s;
                s += name_;
                s += "; track = " + to_string(radians_to_degrees(normalize_angle(point.angle)));
                s += "; process = " + to_string(radians_to_degrees(normalize_angle(filter_->angle())));
                s += "; speed = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_speed())));
                s += "; r = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_r())));
                return s;
        }

        [[nodiscard]] std::string nees_string() const
        {
                std::string s;
                s += "Process " + name_ + " Position: " + nees_position_.check_string();
                s += '\n';
                s += "Process " + name_ + " Angle: " + nees_angle_.check_string();
                s += '\n';
                s += "Process " + name_ + " Angle R: " + nees_angle_r_.check_string();
                return s;
        }

        [[nodiscard]] const std::vector<Vector<2, T>>& position() const
        {
                return position_;
        }

        [[nodiscard]] const std::vector<std::optional<T>>& speed() const
        {
                return speed_;
        }
};
}
