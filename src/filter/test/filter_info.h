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

#include "view/time_point.h"

#include <src/color/rgb8.h>

#include <string>
#include <vector>

namespace ns::filter::test
{
template <std::size_t N, typename T>
struct FilterInfo final
{
        std::string name;
        color::RGB8 color;

        std::vector<view::TimePoint<N, T>> positions;
        std::vector<view::TimePoint<N, T>> positions_p;
        std::vector<view::TimePoint<1, T>> speeds;
        std::vector<view::TimePoint<1, T>> speeds_p;

        FilterInfo(std::string name, color::RGB8 color)
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
}
