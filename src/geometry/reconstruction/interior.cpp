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

/*
Tamal K. Dey.
Curve and Surface Reconstruction: Algorithms with Mathematical Analysis.
Cambridge University Press, 2007.

5 Undersampling
*/

#include "interior.h"

#include "structure.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/geometry/core/delaunay.h>
#include <src/settings/instantiation.h>

#include <cmath>
#include <cstddef>
#include <vector>

namespace ns::geometry::reconstruction
{
namespace
{
// Definition 5.4 (i)
template <std::size_t N>
bool ratio_condition(const ManifoldVertex<N>& vertex, const double rho)
{
        return vertex.radius <= rho * vertex.height;
}

// Definition 5.4 (ii)
template <std::size_t N>
bool normal_condition(const ManifoldVertex<N>& v1, const ManifoldVertex<N>& v2, const double cos_of_alpha)
{
        const double cos_of_angle = dot(v1.positive_norm, v2.positive_norm);
        // the absolute value is used because the positive poles
        // can be in opposite directions for adjacent Voronoi cells
        return std::abs(cos_of_angle) >= cos_of_alpha;
}

template <std::size_t N>
void initial_phase(
        const double rho,
        const double cosine_of_alpha,
        const std::vector<ManifoldVertex<N>>& vertices,
        std::vector<bool>* const interior_vertices,
        std::size_t* const interior_count)
{
        for (std::size_t v = 0; v < vertices.size(); ++v)
        {
                const ManifoldVertex<N>& vertex = vertices[v];

                if (!ratio_condition(vertex, rho))
                {
                        continue;
                }

                const bool flat = [&]
                {
                        for (const auto index : vertex.cocone_neighbors)
                        {
                                if (!normal_condition(vertex, vertices[index], cosine_of_alpha))
                                {
                                        return false;
                                }
                        }
                        return true;
                }();

                if (flat)
                {
                        (*interior_vertices)[v] = true;
                        ++(*interior_count);
                }
        }
}

template <std::size_t N>
void expansion_phase(
        const double rho,
        const double cosine_of_alpha,
        const std::vector<ManifoldVertex<N>>& vertices,
        std::vector<bool>* const interior_vertices,
        std::size_t* const interior_count)
{
        for (std::size_t v = 0; v < vertices.size(); ++v)
        {
                if ((*interior_vertices)[v])
                {
                        continue;
                }

                const ManifoldVertex<N>& vertex = vertices[v];

                if (!ratio_condition(vertex, rho))
                {
                        continue;
                }

                for (const auto index : vertex.cocone_neighbors)
                {
                        if (!(*interior_vertices)[index])
                        {
                                continue;
                        }

                        if (normal_condition(vertex, vertices[index], cosine_of_alpha))
                        {
                                (*interior_vertices)[v] = true;
                                ++(*interior_count);
                                break;
                        }
                }
        }
}

template <std::size_t N>
bool interior_facet(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<ManifoldFacet<N>>& manifold_facets,
        const std::vector<bool>& interior_vertices,
        const int facet)
{
        bool found = false;
        for (std::size_t v = 0; v < N; ++v)
        {
                const bool interior = interior_vertices[delaunay_facets[facet].vertices()[v]];
                const bool interior_cocone = interior && manifold_facets[facet].cocone_vertex[v];
                const bool boundary = !interior;
                if (!(interior_cocone || boundary))
                {
                        return false;
                }
                found = found || interior_cocone;
        }
        return found;
}
}

template <std::size_t N>
std::vector<bool> find_interior_vertices(
        const double rho,
        const double cosine_of_alpha,
        const std::vector<ManifoldVertex<N>>& vertices)
{
        std::vector<bool> interior_vertices(vertices.size(), false);

        std::size_t interior_count = 0;

        initial_phase(rho, cosine_of_alpha, vertices, &interior_vertices, &interior_count);

        LOG("interior_vertices initial phase, interior points count = " + to_string(interior_count)
            + ", vertex count = " + to_string(vertices.size()));

        if (interior_count == 0)
        {
                return interior_vertices;
        }

        while (true)
        {
                std::size_t count = 0;
                expansion_phase(rho, cosine_of_alpha, vertices, &interior_vertices, &count);
                if (count == 0)
                {
                        break;
                }
                interior_count += count;
        }

        LOG("interior_vertices expansion phase, interior point count = " + to_string(interior_count)
            + ", vertex count = " + to_string(vertices.size()));

        return interior_vertices;
}

template <std::size_t N>
std::vector<bool> find_interior_facets(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<ManifoldFacet<N>>& facet_data,
        const std::vector<bool>& interior_vertices)
{
        ASSERT(delaunay_facets.size() == facet_data.size());

        std::vector<bool> res(facet_data.size());
        for (std::size_t i = 0; i < facet_data.size(); ++i)
        {
                res[i] = interior_facet(delaunay_facets, facet_data, interior_vertices, i);
        }
        return res;
}

#define TEMPLATE(N)                                                                                                 \
        template std::vector<bool> find_interior_vertices(double, double, const std::vector<ManifoldVertex<(N)>>&); \
        template std::vector<bool> find_interior_facets(                                                            \
                const std::vector<core::DelaunayFacet<(N)>>&, const std::vector<ManifoldFacet<(N)>>&,               \
                const std::vector<bool>&);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
