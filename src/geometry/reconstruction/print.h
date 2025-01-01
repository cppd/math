/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "structure.h"

#include <src/geometry/core/delaunay.h>

#include <cstddef>
#include <vector>

namespace ns::geometry::reconstruction
{
template <std::size_t N>
void print_delaunay_facets(const std::vector<core::DelaunayFacet<N>>& delaunay_facets);

template <std::size_t N>
void print_cocone_facets(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets);

template <std::size_t N>
void print_non_cocone_facets(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets);

template <std::size_t N>
void print_vertex_data(const std::vector<ManifoldVertex<N>>& vertices);
}
