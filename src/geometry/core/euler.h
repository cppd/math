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

#include <src/com/combinatorics.h>
#include <src/com/hash.h>
#include <src/com/sort.h>

#include <array>
#include <unordered_set>
#include <vector>

namespace ns::geometry
{
template <std::size_t D, std::size_t N>
long long simplex_count(const std::vector<std::array<int, N>>& facets)
{
        static_assert(D < N);

        struct Hash
        {
                std::size_t operator()(const std::array<int, D + 1>& v) const
                {
                        return compute_hash(v);
                }
        };

        std::unordered_set<std::array<int, D + 1>, Hash> simplex_set;

        for (std::array<int, N> facet : facets)
        {
                facet = sort(std::move(facet));
                for (const std::array<unsigned char, D + 1>& simplex : COMBINATIONS<N, D + 1>)
                {
                        std::array<int, D + 1> simplex_vertex_indices;
                        for (unsigned i = 0; i < D + 1; ++i)
                        {
                                simplex_vertex_indices[i] = facet[simplex[i]];
                        }
                        simplex_set.insert(simplex_vertex_indices);
                }
        }

        return simplex_set.size();
}

namespace euler_characteristic_implementation
{
template <std::size_t N, std::size_t... I>
int euler_characteristic(const std::vector<std::array<int, N>>& facets, std::integer_sequence<std::size_t, I...>&&)
{
        static_assert(sizeof...(I) == N);

        return ((((I & 1) ? -1 : 1) * simplex_count<I>(facets)) + ...);
}

template <std::size_t N, std::size_t... I>
std::array<long long, N> simplex_counts(
        const std::vector<std::array<int, N>>& facets,
        std::integer_sequence<std::size_t, I...>&&)
{
        static_assert(sizeof...(I) == N);

        return {simplex_count<I>(facets)...};
}
}

template <std::size_t N>
int euler_characteristic(const std::vector<std::array<int, N>>& facets)
{
        namespace impl = euler_characteristic_implementation;

        return impl::euler_characteristic(facets, std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N>
std::array<long long, N> simplex_counts(const std::vector<std::array<int, N>>& facets)
{
        namespace impl = euler_characteristic_implementation;

        return impl::simplex_counts(facets, std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N>
constexpr int euler_characteristic_for_convex_polytope()
{
        return (N & 1) ? 2 : 0;
}
}
