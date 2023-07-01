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

#include <src/com/combinatorics.h>
#include <src/com/hash.h>
#include <src/com/sort.h>

#include <array>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ns::geometry::core
{
namespace euler_implementation
{
template <std::size_t SIMPLEX_DIMENSION, std::size_t N>
long long simplex_count(const std::vector<std::array<int, N>>& facets)
{
        static_assert(SIMPLEX_DIMENSION < N);

        constexpr std::size_t VERTEX_COUNT = SIMPLEX_DIMENSION + 1;

        struct Hash final
        {
                std::size_t operator()(const std::array<int, VERTEX_COUNT>& v) const
                {
                        return compute_hash(v);
                }
        };

        std::unordered_set<std::array<int, VERTEX_COUNT>, Hash> simplex_set;

        for (std::array<int, N> facet : facets)
        {
                facet = sort(std::move(facet));
                for (const std::array<unsigned char, VERTEX_COUNT>& simplex : COMBINATIONS<N, VERTEX_COUNT>)
                {
                        std::array<int, VERTEX_COUNT> simplex_vertex_indices;
                        for (std::size_t i = 0; i < VERTEX_COUNT; ++i)
                        {
                                simplex_vertex_indices[i] = facet[simplex[i]];
                        }
                        simplex_set.insert(simplex_vertex_indices);
                }
        }

        return simplex_set.size();
}
}

template <std::size_t N>
int euler_characteristic(const std::vector<std::array<int, N>>& facets)
{
        namespace impl = euler_implementation;

        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                return (((I & 1) ? -impl::simplex_count<I>(facets) : impl::simplex_count<I>(facets)) + ...);
        }(std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N>
std::array<long long, N> simplex_counts(const std::vector<std::array<int, N>>& facets)
{
        namespace impl = euler_implementation;
        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                return std::array<long long, N>{impl::simplex_count<I>(facets)...};
        }(std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N>
constexpr int euler_characteristic_for_convex_polytope()
{
        return (N & 1) ? 2 : 0;
}
}
