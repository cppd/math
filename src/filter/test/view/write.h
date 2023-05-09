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
#include <string_view>
#include <vector>

namespace ns::filter::test::view
{
template <std::size_t N, typename T>
void write_to_file(
        const std::string_view annotation,
        const Track<N, T>& track,
        const std::vector<std::optional<Vector<N, T>>>& position,
        const std::vector<std::optional<T>>& speed,
        const std::vector<Vector<N, T>>& process);
}
