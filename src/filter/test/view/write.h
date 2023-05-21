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

#include "../simulator.h"

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
        unsigned char color;
        std::vector<Vector<N, T>> speed;
        std::vector<Vector<N, T>> position;
};

template <std::size_t N, typename T>
void write_to_file(
        std::string_view annotation,
        const Track<N, T>& track,
        std::size_t track_position_interval,
        const std::vector<std::optional<Vector<N, T>>>& lkf_speed,
        const std::vector<std::optional<Vector<N, T>>>& lkf_position,
        const std::vector<Filter<N, T>>& filters);
}
