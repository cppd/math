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

#include "../core/delaunay.h"

#include <src/com/arrays.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::geometry::reconstruction
{
template <std::size_t N>
struct ManifoldVertex final
{
        Vector<N, double> positive_norm;
        double height;
        double radius;
        std::vector<int> cocone_neighbors;

        ManifoldVertex(const Vector<N, double>& positive_norm, const double height, const double radius)
                : positive_norm(positive_norm),
                  height(height),
                  radius(radius)
        {
        }
};

template <std::size_t N>
struct ManifoldFacet final
{
        std::array<bool, N> cocone_vertex{make_array_value<bool, N>(false)};
};

template <std::size_t N>
struct ManifoldData final
{
        std::vector<ManifoldVertex<N>> vertices;
        std::vector<ManifoldFacet<N>> facets;
};

template <std::size_t N>
[[nodiscard]] ManifoldData<N> find_manifold_data(
        bool find_cocone_neighbors,
        const std::vector<Vector<N, double>>& points,
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets);
}
