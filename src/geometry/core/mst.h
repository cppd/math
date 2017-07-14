/*
Copyright (C) 2017 Topological Manifold

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

#include "delaunay.h"

template <size_t N>
void minimal_spanning_tree(const std::vector<vec<N>>& points, const std::vector<DelaunayObject<N>>& delaunay_objects);

// clang-format off
extern template
void minimal_spanning_tree(const std::vector<vec<2>>& points, const std::vector<DelaunayObject<2>>& delaunay_objects);
extern template
void minimal_spanning_tree(const std::vector<vec<3>>& points, const std::vector<DelaunayObject<3>>& delaunay_objects);
extern template
void minimal_spanning_tree(const std::vector<vec<4>>& points, const std::vector<DelaunayObject<4>>& delaunay_objects);
// clang-format on
