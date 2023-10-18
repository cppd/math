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
template <std::size_t N, typename T>
struct FilterData final
{
        std::string name;
        color::RGB8 color;

        std::vector<TimePoint<N, T>> positions;
        std::vector<TimePoint<N, T>> positions_p;
        std::vector<TimePoint<1, T>> speeds;
        std::vector<TimePoint<1, T>> speeds_p;

        FilterData(std::string name, color::RGB8 color)
                : name(std::move(name)),
                  color(color)
        {
        }

        void update(const T time, const auto& update)
        {
                if (!update)
                {
                        return;
                }

                positions.push_back({.time = time, .point = update->position});
                positions_p.push_back({.time = time, .point = update->position_p});
                speeds.push_back({.time = time, .point = Vector<1, T>(update->speed)});
                speeds_p.push_back({.time = time, .point = Vector<1, T>(update->speed_p)});
        }
};

template <std::size_t N, typename T>
struct TestFilter final
{
        std::unique_ptr<Filter<N, T>> filter;
        FilterData<N, T> data;

        TestFilter(std::unique_ptr<Filter<N, T>>&& filter, std::string name, color::RGB8 color)
                : filter(std::move(filter)),
                  data(std::move(name), color)
        {
        }
};
}
