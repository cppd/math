/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "convex_hull.h"
#include "ridge.h"
#include "voronoi.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <array>
#include <unordered_map>
#include <vector>

namespace ns::geometry
{
template <std::size_t N>
class DelaunayFacet final
{
        const std::array<int, N> vertices_;
        // the vector is directed outside if there is only one Delaunay object
        const Vector<N, double> ortho_;
        // the second element is negative if there is only one Delaunay object
        const std::array<int, 2> delaunay_;

public:
        DelaunayFacet(
                const std::array<int, N>& vertices,
                const Vector<N, double>& ortho,
                const int delaunay_0,
                const int delaunay_1)
                : vertices_(vertices), ortho_(ortho), delaunay_{delaunay_0, delaunay_1}
        {
        }
        bool one_sided() const
        {
                return delaunay_[1] < 0;
        }
        const std::array<int, N>& vertices() const
        {
                return vertices_;
        }
        const Vector<N, double>& ortho() const
        {
                return ortho_;
        }
        int delaunay(const unsigned i) const
        {
                ASSERT(i == 0 || (i == 1 && delaunay_[1] >= 0));
                return delaunay_[i];
        }
};

template <std::size_t N>
class DelaunayObject final
{
        const std::array<int, N + 1> vertices_;
        const Vector<N, double> voronoi_vertex_;

public:
        DelaunayObject(const std::array<int, N + 1>& vertices, const Vector<N, double>& voronoi_vertex)
                : vertices_(vertices), voronoi_vertex_(voronoi_vertex)
        {
        }
        const std::array<int, N + 1>& vertices() const
        {
                return vertices_;
        }
        const Vector<N, double>& voronoi_vertex() const
        {
                return voronoi_vertex_;
        }
};

template <std::size_t N>
void create_delaunay_objects_and_facets(
        const std::vector<Vector<N, double>>& points,
        const std::vector<DelaunaySimplex<N>>& simplices,
        std::vector<DelaunayObject<N>>* const delaunay_objects,
        std::vector<DelaunayFacet<N>>* const delaunay_facets)
{
        static constexpr int NULL_INDEX = -1;

        std::unordered_map<const DelaunaySimplex<N>*, int> simplex_to_delaunay(simplices.size());

        delaunay_objects->clear();
        delaunay_objects->reserve(simplices.size());
        simplex_to_delaunay.reserve(simplices.size());
        for (int i = 0; const DelaunaySimplex<N>& simplex : simplices)
        {
                delaunay_objects->emplace_back(
                        simplex.vertices(), compute_voronoi_vertex_for_delaunay_object(points, simplex.vertices()));
                simplex_to_delaunay.emplace(&simplex, i++);
        }

        const auto delaunay_index_for_simplex = [&simplex_to_delaunay](const DelaunaySimplex<N>* const simplex)
        {
                const auto iter = simplex_to_delaunay.find(simplex);
                ASSERT(iter != simplex_to_delaunay.cend());
                return iter->second;
        };

        std::unordered_map<Ridge<N + 1>, RidgeData2<DelaunaySimplex<N>>> facets(simplices.size());
        for (const DelaunaySimplex<N>& simplex : simplices)
        {
                add_to_ridges(simplex, &facets);
        }

        delaunay_facets->clear();
        delaunay_facets->reserve(facets.size());
        for (const auto& f : facets)
        {
                const Ridge<N + 1>& facet = f.first;
                const RidgeData2<DelaunaySimplex<N>>& facet_data = f.second;

                ASSERT(facet_data.size() == 1 || facet_data.size() == 2);

                if (facet_data.size() == 1)
                {
                        const int index = facet_data[0].facet() ? 0 : 1;
                        const DelaunaySimplex<N>* const simplex = facet_data[index].facet();
                        const Vector<N, double> ortho = simplex->ortho(facet_data[index].vertex_index());

                        delaunay_facets->emplace_back(
                                facet.vertices(), ortho, delaunay_index_for_simplex(simplex), NULL_INDEX);
                }
                else
                {
                        const DelaunaySimplex<N>* const simplex_0 = facet_data[0].facet();
                        const DelaunaySimplex<N>* const simplex_1 = facet_data[1].facet();

                        const Vector<N, double> ortho = simplex_0->ortho(facet_data[0].vertex_index());
                        ASSERT(ortho == -simplex_1->ortho(facet_data[1].vertex_index()));

                        delaunay_facets->emplace_back(
                                facet.vertices(), ortho, delaunay_index_for_simplex(simplex_0),
                                delaunay_index_for_simplex(simplex_1));
                }
        }
}
}
