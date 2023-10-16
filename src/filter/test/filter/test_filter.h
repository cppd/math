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

#include "estimation.h"
#include "filter.h"
#include "measurement.h"
#include "time_point.h"

#include <src/color/rgb8.h>

#include <memory>
#include <string>
#include <vector>

namespace ns::filter::test::filter
{
template <typename T>
class TestFilter final
{
        std::unique_ptr<Filter<T>> filter_;
        std::string name_;
        color::RGB8 color_;
        std::vector<TimePoint<2, T>> positions_;
        std::vector<TimePoint<2, T>> positions_p_;
        std::vector<TimePoint<1, T>> speeds_;
        std::vector<TimePoint<1, T>> speeds_p_;

public:
        TestFilter(std::unique_ptr<Filter<T>>&& filter, std::string name, color::RGB8 color)
                : filter_(std::move(filter)),
                  name_(std::move(name)),
                  color_(color)
        {
        }

        void update(const Measurements<2, T>& m, const Estimation<T>& estimation)
        {
                const auto update = filter_->update(m, estimation);
                if (!update)
                {
                        return;
                }

                positions_.push_back({.time = m.time, .point = update->position});
                positions_p_.push_back({.time = m.time, .point = update->position_p});
                speeds_.push_back({.time = m.time, .point = Vector<1, T>(update->speed)});
                speeds_p_.push_back({.time = m.time, .point = Vector<1, T>(update->speed_p)});
        }

        [[nodiscard]] const std::string& name() const
        {
                return name_;
        }

        [[nodiscard]] color::RGB8 color() const
        {
                return color_;
        }

        [[nodiscard]] std::string consistency_string() const
        {
                return filter_->consistency_string(name_);
        }

        [[nodiscard]] const std::vector<TimePoint<2, T>>& positions() const
        {
                return positions_;
        }

        [[nodiscard]] const std::vector<TimePoint<2, T>>& positions_p() const
        {
                return positions_p_;
        }

        [[nodiscard]] const std::vector<TimePoint<1, T>>& speeds() const
        {
                return speeds_;
        }

        [[nodiscard]] const std::vector<TimePoint<1, T>>& speeds_p() const
        {
                return speeds_p_;
        }
};
}
