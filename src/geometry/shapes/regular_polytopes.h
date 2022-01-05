/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/numerical/complement.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <vector>

// Origin-centered

namespace ns::geometry
{
template <std::size_t N, typename T>
std::array<Vector<N, T>, N + 1> create_simplex()
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        const Vector<N + 1, T> simplex_hyperplane_normal = Vector<N + 1, T>(1 / std::sqrt(T(N + 1)));

        const std::array<Vector<N + 1, T>, N> basis =
                numerical::orthogonal_complement_of_unit_vector(simplex_hyperplane_normal);

        std::array<Vector<N + 1, T>, N + 1> vertices;
        for (unsigned i = 0; i < N + 1; ++i)
        {
                vertices[i] = Vector<N + 1, T>(0);
        }
        for (unsigned i = 0; i < N + 1; ++i)
        {
                vertices[i][i] = 1;
        }

        std::array<Vector<N, T>, N + 1> result;
        for (unsigned i = 0; i < N + 1; ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        result[i][n] = dot(vertices[i], basis[n]);
                }
                result[i].normalize();
        }
        return result;
}

namespace regular_polytopes_implementation
{
template <unsigned I, std::size_t N, typename T>
void create_cross_polytope_facets(Vector<N, T>* const point, std::vector<std::array<Vector<N, T>, N>>* const facets)
{
        static_assert(I <= N);
        if constexpr (I == N)
        {
                std::array<Vector<N, T>, N>& facet = facets->emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        facet[i] = Vector<N, T>(0);
                        facet[i][i] = (*point)[i];
                }
        }
        else
        {
                (*point)[I] = -1;
                create_cross_polytope_facets<I + 1>(point, facets);
                (*point)[I] = 1;
                create_cross_polytope_facets<I + 1>(point, facets);
        }
}
}

template <std::size_t N, typename T>
std::vector<std::array<Vector<N, T>, N>> create_cross_polytope()
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        constexpr unsigned FACET_COUNT = 1 << N;

        std::vector<std::array<Vector<N, T>, N>> facets;
        facets.reserve(FACET_COUNT);

        Vector<N, T> point;
        regular_polytopes_implementation::create_cross_polytope_facets<0>(&point, &facets);
        ASSERT(facets.size() == FACET_COUNT);

        return facets;
}

template <typename T>
std::vector<std::array<Vector<3, T>, 3>> create_icosahedron()
{
        static_assert(std::is_floating_point_v<T>);

        constexpr unsigned FACET_COUNT = 20;
        constexpr unsigned VERTEX_COUNT = 12;

        const T p = (1 + std::sqrt(T(5))) / 2;

        std::vector<Vector<3, T>> vertices;
        vertices.reserve(VERTEX_COUNT);

        vertices.emplace_back(-1, p, 0);
        vertices.emplace_back(1, p, 0);
        vertices.emplace_back(-1, -p, 0);
        vertices.emplace_back(1, -p, 0);
        vertices.emplace_back(0, -1, p);
        vertices.emplace_back(0, 1, p);
        vertices.emplace_back(0, -1, -p);
        vertices.emplace_back(0, 1, -p);
        vertices.emplace_back(p, 0, -1);
        vertices.emplace_back(p, 0, 1);
        vertices.emplace_back(-p, 0, -1);
        vertices.emplace_back(-p, 0, 1);

        for (Vector<3, T>& v : vertices)
        {
                v.normalize();
        }

        std::vector<std::array<Vector<3, T>, 3>> facets;
        facets.reserve(FACET_COUNT);

        facets.push_back({vertices[0], vertices[1], vertices[7]});
        facets.push_back({vertices[0], vertices[5], vertices[1]});
        facets.push_back({vertices[0], vertices[7], vertices[10]});
        facets.push_back({vertices[0], vertices[10], vertices[11]});
        facets.push_back({vertices[0], vertices[11], vertices[5]});
        facets.push_back({vertices[1], vertices[5], vertices[9]});
        facets.push_back({vertices[2], vertices[4], vertices[11]});
        facets.push_back({vertices[3], vertices[2], vertices[6]});
        facets.push_back({vertices[3], vertices[4], vertices[2]});
        facets.push_back({vertices[3], vertices[6], vertices[8]});
        facets.push_back({vertices[3], vertices[8], vertices[9]});
        facets.push_back({vertices[3], vertices[9], vertices[4]});
        facets.push_back({vertices[4], vertices[9], vertices[5]});
        facets.push_back({vertices[5], vertices[11], vertices[4]});
        facets.push_back({vertices[6], vertices[2], vertices[10]});
        facets.push_back({vertices[7], vertices[1], vertices[8]});
        facets.push_back({vertices[8], vertices[6], vertices[7]});
        facets.push_back({vertices[9], vertices[8], vertices[1]});
        facets.push_back({vertices[10], vertices[7], vertices[6]});
        facets.push_back({vertices[11], vertices[10], vertices[2]});

        return facets;
}
}
