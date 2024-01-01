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

#include <src/numerical/vector.h>
#include <src/progress/progress.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::geometry::graph
{
template <std::size_t N>
std::vector<std::array<int, 2>> minimum_spanning_tree(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N + 1>>& delaunay_objects,
        progress::Ratio* progress);
}
