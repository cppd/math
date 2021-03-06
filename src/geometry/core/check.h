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

#include "euler.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/print.h>
#include <src/com/sort.h>
#include <src/numerical/orthogonal.h>
#include <src/numerical/vec.h>

#include <array>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns::geometry
{
namespace check_implementation
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
                        error(name + " facet unique vertex count " + to_string(facet_vertex_set.size())
                              + " is not equal to " + to_string(N));
                }

                Vector<N, T> n = numerical::ortho_nn(vertices, facet).normalized();
                if (!is_finite(n))
                {
                        error(name + " facet normal " + to_string(n) + " is not finite");
                }
        }
}

template <std::size_t N>
void check_manifoldness(const std::string& name, const std::vector<std::array<int, N>>& facets, bool has_boundary)
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

        if (!has_boundary)
        {
                for (const auto& [ridge, count] : ridges)
                {
                        if (count != 2)
                        {
                                error(name + " ridge facet count " + to_string(count) + " is not equal to 2");
                        }
                }
        }
        else
        {
                for (const auto& [ridge, count] : ridges)
                {
                        if (count > 2)
                        {
                                error(name + " ridge facet count " + to_string(count) + " is greater than 2");
                        }
                }
        }
}

template <std::size_t N>
void check_euler_characteristic(
        const std::string& name,
        const std::vector<std::array<int, N>>& facets,
        int expected_euler_characteristic)
{
        const int mesh_euler_characteristic = euler_characteristic(facets);

        if (mesh_euler_characteristic == expected_euler_characteristic)
        {
                return;
        }

        std::ostringstream oss;

        oss << name << " Euler characteristic (" << mesh_euler_characteristic << ")";
        oss << " is not equal to " << expected_euler_characteristic;

        std::array<long long, N> counts = simplex_counts(facets);
        for (unsigned i = 0; i < N; ++i)
        {
                oss << '\n' << i << "-simplex count = " << counts[i];
        }

        error(oss.str());
}
}

template <std::size_t N, typename T>
void check_mesh(
        const std::string& name,
        const std::vector<Vector<N, T>>& vertices,
        const std::vector<std::array<int, N>>& facets,
        bool has_boundary,
        const std::optional<int>& expected_euler_characteristic)
{
        namespace impl = check_implementation;

        impl::check_facet_dimension(name, vertices, facets);

        impl::check_manifoldness(name, facets, has_boundary);

        if (expected_euler_characteristic)
        {
                impl::check_euler_characteristic(name, facets, *expected_euler_characteristic);
        }
}
}
