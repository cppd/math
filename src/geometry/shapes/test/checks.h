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

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/print.h>
#include <src/com/sort.h>
#include <src/geometry/core/euler.h>
#include <src/numerical/orthogonal.h>
#include <src/numerical/vec.h>

#include <array>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns::geometry::shapes::test
{
template <std::size_t N, typename T>
void check_facet_dimension(
        const std::string& name,
        const std::vector<Vector<N, T>>& vertices,
        const std::vector<std::array<int, N>>& facets)
{
        std::unordered_set<Vector<N, T>> facet_vertex_set;
        for (const std::array<int, N>& facet : facets)
        {
                facet_vertex_set.clear();
                for (int vertex_index : facet)
                {
                        facet_vertex_set.insert(vertices[vertex_index]);
                }
                if (facet_vertex_set.size() != N)
                {
                        error(name + " facet vertex count " + to_string(facet_vertex_set.size()) + " is not equal to "
                              + to_string(N));
                }

                Vector<N, T> n = numerical::ortho_nn(vertices, facet).normalized();
                if (!is_finite(n))
                {
                        error(name + " facet normal " + to_string(n) + " is not finite");
                }
        }
}

template <std::size_t N>
void check_manifoldness(const std::string& name, const std::vector<std::array<int, N>>& facets)
{
        struct Hash
        {
                std::size_t operator()(const std::array<int, N - 1>& v) const
                {
                        return array_hash(v);
                }
        };

        std::unordered_map<std::array<int, N - 1>, int, Hash> ridges;
        for (const std::array<int, N>& facet : facets)
        {
                for (unsigned r = 0; r < N; ++r)
                {
                        std::array<int, N - 1> ridge = sort(del_elem(facet, r));
                        ++ridges[ridge];
                }
        }

        for (const auto& [ridge, count] : ridges)
        {
                if (count != 2)
                {
                        error(name + " ridge facet count " + to_string(count) + " is not equal to 2");
                }
        }
}

template <std::size_t N>
void check_euler_characteristic(const std::string& name, const std::vector<std::array<int, N>>& facets)
{
        constexpr int EXPECTED_EULER_CHARACTERISTIC = euler_characteristic_for_convex_polytope<N>();

        const int computed_euler_characteristic = euler_characteristic(facets);

        if (computed_euler_characteristic == EXPECTED_EULER_CHARACTERISTIC)
        {
                return;
        }

        std::ostringstream oss;

        oss << name << " Euler characteristic (" << computed_euler_characteristic << ")";
        oss << " is not equal to " << EXPECTED_EULER_CHARACTERISTIC;

        std::array<long long, N> counts = simplex_counts(facets);
        for (unsigned i = 0; i < N; ++i)
        {
                oss << '\n' << i << "-simplex count = " << counts[i];
        }

        error(oss.str());
}
}
