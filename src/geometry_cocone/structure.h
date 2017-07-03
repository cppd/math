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

#include "geometry/delaunay.h"
#include "geometry/vec.h"
#include "geometry/vec_array.h"

#include <vector>

template <size_t N>
struct SurfaceVertex
{
        const vec<N> positive_norm;
        // const vec<N> negative_pole;
        const double height;
        const double radius;
        std::vector<int> cocone_neighbors;

        SurfaceVertex(const vec<N>& positive_norm_, double height_, double radius_)
                : positive_norm(positive_norm_), height(height_), radius(radius_)
        {
        }
};

template <size_t N>
struct SurfaceFacet
{
        std::array<bool, N> cocone_vertex;

        SurfaceFacet() : cocone_vertex(make_array_value<bool, N>(false))
        {
        }
};

template <size_t N>
void vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<N>>& points,
                           const std::vector<DelaunayObject<N>>& delaunay_objects,
                           const std::vector<DelaunayFacet<N>>& delaunay_facets, std::vector<SurfaceVertex<N>>* vertex_data,
                           std::vector<SurfaceFacet<N>>* facet_data);

// clang-format off
extern template
void vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<2>>& points,
                           const std::vector<DelaunayObject<2>>& delaunay_objects,
                           const std::vector<DelaunayFacet<2>>& delaunay_facets, std::vector<SurfaceVertex<2>>* vertex_data,
                           std::vector<SurfaceFacet<2>>* facet_data);
extern template
void vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<3>>& points,
                           const std::vector<DelaunayObject<3>>& delaunay_objects,
                           const std::vector<DelaunayFacet<3>>& delaunay_facets, std::vector<SurfaceVertex<3>>* vertex_data,
                           std::vector<SurfaceFacet<3>>* facet_data);
extern template
void vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<4>>& points,
                           const std::vector<DelaunayObject<4>>& delaunay_objects,
                           const std::vector<DelaunayFacet<4>>& delaunay_facets, std::vector<SurfaceVertex<4>>* vertex_data,
                           std::vector<SurfaceFacet<4>>* facet_data);
// clang-format on
