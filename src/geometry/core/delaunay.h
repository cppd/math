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
#include <src/numerical/vec.h>

#include <array>
#include <unordered_map>
#include <vector>

namespace ns::geometry
{
template <std::size_t N>
class DelaunayFacet
{
        const std::array<int, N> m_vertices;
        // the vector is directed outside if there is only one Delaunay object
        const Vector<N, double> m_ortho;
        // the second element is negative if there is only one Delaunay object
        const std::array<int, 2> m_delaunay;

public:
        DelaunayFacet(
                const std::array<int, N>& vertices,
                const Vector<N, double>& ortho,
                int delaunay_0,
                int delaunay_1)
                : m_vertices(vertices), m_ortho(ortho), m_delaunay{delaunay_0, delaunay_1}
        {
        }
        bool one_sided() const
        {
                return m_delaunay[1] < 0;
        }
        const std::array<int, N>& vertices() const
        {
                return m_vertices;
        }
        const Vector<N, double>& ortho() const
        {
                return m_ortho;
        }
        int delaunay(unsigned i) const
        {
                ASSERT(i == 0 || (i == 1 && m_delaunay[1] >= 0));
                return m_delaunay[i];
        }
};

template <std::size_t N>
class DelaunayObject
{
        const std::array<int, N + 1> m_vertices;
        const Vector<N, double> m_voronoi_vertex;

public:
        DelaunayObject(const std::array<int, N + 1>& vertices, const Vector<N, double>& voronoi_vertex)
                : m_vertices(vertices), m_voronoi_vertex(voronoi_vertex)
        {
        }
        const std::array<int, N + 1>& vertices() const
        {
                return m_vertices;
        }
        const Vector<N, double>& voronoi_vertex() const
        {
                return m_voronoi_vertex;
        }
};

template <std::size_t N>
void create_delaunay_objects_and_facets(
        const std::vector<Vector<N, double>>& points,
        const std::vector<DelaunaySimplex<N>>& simplices,
        std::vector<DelaunayObject<N>>* delaunay_objects,
        std::vector<DelaunayFacet<N>>* delaunay_facets)
{
        static constexpr int NULL_INDEX = -1;

        std::unordered_map<const DelaunaySimplex<N>*, int> simplex_to_delaunay(simplices.size());

        delaunay_objects->clear();
        delaunay_objects->reserve(simplices.size());
        int delaunay_index = 0;
        for (const DelaunaySimplex<N>& simplex : simplices)
        {
                delaunay_objects->emplace_back(
                        simplex.vertices(), compute_voronoi_vertex_for_delaunay_object(points, simplex.vertices()));
                simplex_to_delaunay.emplace(&simplex, delaunay_index++);
        }

        std::unordered_map<Ridge<N + 1>, RidgeData2<DelaunaySimplex<N>>> facets(simplices.size());

        for (const DelaunaySimplex<N>& simplex : simplices)
        {
                add_to_ridges(simplex, &facets);
        }

        delaunay_facets->clear();
        delaunay_facets->reserve(facets.size());

        for (auto r_i = facets.cbegin(); r_i != facets.cend(); ++r_i)
        {
                const Ridge<N + 1>& facet = r_i->first;
                const RidgeData2<DelaunaySimplex<N>>& facet_data = r_i->second;

                ASSERT(facet_data.size() == 1 || facet_data.size() == 2);

                if (facet_data.size() == 1)
                {
                        int index = facet_data[0].facet() ? 0 : 1;
                        const DelaunaySimplex<N>* simplex = facet_data[index].facet();
                        Vector<N, double> facet_ortho = simplex->ortho(facet_data[index].vertex_index());

                        auto delaunay_i = simplex_to_delaunay.find(simplex);

                        ASSERT(delaunay_i != simplex_to_delaunay.cend());

                        delaunay_facets->emplace_back(facet.vertices(), facet_ortho, delaunay_i->second, NULL_INDEX);
                }
                else
                {
                        const DelaunaySimplex<N>* simplex_0 = facet_data[0].facet();
                        const DelaunaySimplex<N>* simplex_1 = facet_data[1].facet();

                        Vector<N, double> facet_ortho = simplex_0->ortho(facet_data[0].vertex_index());
                        ASSERT(facet_ortho == -simplex_1->ortho(facet_data[1].vertex_index()));

                        auto delaunay_i_0 = simplex_to_delaunay.find(simplex_0);
                        auto delaunay_i_1 = simplex_to_delaunay.find(simplex_1);

                        ASSERT(delaunay_i_0 != simplex_to_delaunay.cend()
                               && delaunay_i_1 != simplex_to_delaunay.cend());

                        delaunay_facets->emplace_back(
                                facet.vertices(), facet_ortho, delaunay_i_0->second, delaunay_i_1->second);
                }
        }
}
}
