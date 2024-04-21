/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/color/rgb8.h>
#include <src/filter/core/test/measurements.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ns::filter::core::test::view
{
template <typename T>
struct Point final
{
        T time;
        T x;
        T x_stddev;
        T v;
        T v_stddev;
};

template <typename T>
struct Filter final
{
        std::string name;
        color::RGB8 color;
        std::vector<Point<T>> points;

        Filter(std::string name, color::RGB8 color, std::vector<Point<T>> points)
                : name(std::move(name)),
                  color(color),
                  points(std::move(points))
        {
        }
};

template <typename T>
void write(
        const std::string_view& file_name,
        const std::vector<Measurements<T>>& measurements,
        T interval,
        const std::vector<Filter<T>>& filters);
}
