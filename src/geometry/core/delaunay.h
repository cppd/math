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

#include "convex_hull.h"
#include "ridge.h"
#include "voronoi.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <array>
#include <unordered_map>
#include <vector>

namespace ns::geometry::core
{
template <std::size_t N>
class DelaunayFacet final
{
        std::array<int, N> vertices_;

        // the vector is directed outside if there is only one Delaunay object
        Vector<N, double> ortho_;

        // the second element is negative if there is only one Delaunay object
        std::array<int, 2> delaunay_;

public:
        DelaunayFacet(
                const std::array<int, N>& vertices,
                const Vector<N, double>& ortho,
                const int delaunay_0,
                const int delaunay_1 = -1)
                : vertices_(vertices),
                  ortho_(ortho),
                  delaunay_{delaunay_0, delaunay_1}
        {
        }

        [[nodiscard]] bool one_sided() const
        {
                return delaunay_[1] < 0;
        }

        [[nodiscard]] const std::array<int, N>& vertices() const
        {
                return vertices_;
        }

        [[nodiscard]] const Vector<N, double>& ortho() const
        {
                return ortho_;
        }

        [[nodiscard]] int delaunay(const unsigned index) const
        {
                ASSERT(index == 0 || (index == 1 && delaunay_[1] >= 0));
                return delaunay_[index];
        }
};

template <std::size_t N>
class DelaunayObject final
{
        std::array<int, N + 1> vertices_;
        Vector<N, double> voronoi_vertex_;

public:
        DelaunayObject(const std::array<int, N + 1>& vertices, const Vector<N, double>& voronoi_vertex)
                : vertices_(vertices),
                  voronoi_vertex_(voronoi_vertex)
        {
        }

        [[nodiscard]] const std::array<int, N + 1>& vertices() const
        {
                return vertices_;
        }

        [[nodiscard]] const Vector<N, double>& voronoi_vertex() const
        {
                return voronoi_vertex_;
        }
};

template <std::size_t N>
[[nodiscard]] std::vector<DelaunayObject<N>> create_delaunay_objects(
        const std::vector<Vector<N, double>>& points,
        const std::vector<DelaunaySimplex<N>>& simplices)
{
        std::vector<DelaunayObject<N>> res;
        res.reserve(simplices.size());
        for (const DelaunaySimplex<N>& simplex : simplices)
        {
                res.emplace_back(
                        simplex.vertices(), compute_voronoi_vertex_for_delaunay_object(points, simplex.vertices()));
        }
        return res;
}

template <std::size_t N>
[[nodiscard]] std::vector<DelaunayFacet<N>> create_delaunay_facets(const std::vector<DelaunaySimplex<N>>& simplices)
{
        const std::unordered_map<const DelaunaySimplex<N>*, int> simplex_to_delaunay_map = [&]
        {
                std::unordered_map<const DelaunaySimplex<N>*, int> res(simplices.size());
                for (int i = 0; const DelaunaySimplex<N>& simplex : simplices)
                {
                        res.emplace(&simplex, i++);
                }
                return res;
        }();

        const auto simplex_to_delaunay = [&simplex_to_delaunay_map](const DelaunaySimplex<N>* const simplex)
        {
                const auto iter = simplex_to_delaunay_map.find(simplex);
                ASSERT(iter != simplex_to_delaunay_map.cend());
                return iter->second;
        };

        const std::unordered_map<Ridge<N + 1>, RidgeFacets2<DelaunaySimplex<N>>> ridges = [&]
        {
                std::unordered_map<Ridge<N + 1>, RidgeFacets2<DelaunaySimplex<N>>> res(simplices.size());
                for (const DelaunaySimplex<N>& simplex : simplices)
                {
                        add_to_ridges(&simplex, &res);
                }
                return res;
        }();

        std::vector<DelaunayFacet<N>> res;
        res.reserve(ridges.size());

        for (const auto& [ridge, ridge_facets] : ridges)
        {
                ASSERT(ridge_facets.f0().facet());

                const DelaunaySimplex<N>* const simplex_0 = ridge_facets.f0().facet();
                const Vector<N, double> ortho = simplex_0->ortho(ridge_facets.f0().vertex_index());
                const int delaunay_0 = simplex_to_delaunay(simplex_0);

                if (!ridge_facets.f1().facet())
                {
                        res.emplace_back(ridge.vertices(), ortho, delaunay_0);
                        continue;
                }

                const DelaunaySimplex<N>* const simplex_1 = ridge_facets.f1().facet();
                ASSERT(ortho == -simplex_1->ortho(ridge_facets.f1().vertex_index()));

                const int delaunay_1 = simplex_to_delaunay(simplex_1);
                res.emplace_back(ridge.vertices(), ortho, delaunay_0, delaunay_1);
        }

        return res;
}
}
