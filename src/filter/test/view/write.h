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

#include "../measurement.h"
#include "../time_point.h"

#include <src/color/rgb8.h>
#include <src/numerical/vector.h>

#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test::view
{
template <std::size_t N, typename T>
struct Filter final
{
        std::string name;
        color::RGB8 color;
        std::vector<TimePoint<1, T>> speed;
        std::vector<TimePoint<1, T>> speed_p;
        std::vector<TimePoint<N, T>> position;
        std::vector<TimePoint<N, T>> position_p;
};

template <std::size_t N, typename T>
void write_to_file(
        std::string_view annotation,
        const std::vector<Measurements<N, T>>& measurements,
        T interval,
        const std::vector<Filter<N, T>>& filters);
}
