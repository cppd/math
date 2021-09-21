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

#include <src/com/error.h>
#include <src/numerical/vec.h>

#include <array>
#include <cmath>
#include <random>

namespace ns::geometry::spatial::test
{
namespace parallelotope_points_implementation
{
template <typename T>
inline constexpr T MAX_DOT_PRODUCT_OF_EDGES;
template <>
inline constexpr double MAX_DOT_PRODUCT_OF_EDGES<double> = 0.9;

template <std::size_t N, typename T>
bool test_edges(const T edge_min_length, const T edge_max_length, const std::array<Vector<N, T>, N>& edges)
{
        std::array<Vector<N, T>, N> unit_edges = edges;
        for (Vector<N, T>& v : unit_edges)
        {
                T length = v.norm();
                if (!(length >= edge_min_length && length <= edge_max_length))
                {
                        return false;
                }
                v /= length;
        }

        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = i + 1; j < N; ++j)
                {
                        if (!(std::abs(dot(unit_edges[i], unit_edges[j])) < MAX_DOT_PRODUCT_OF_EDGES<T>))
                        {
                                return false;
                        }
                }
        }

        return true;
}
}

template <std::size_t N, typename T, typename Engine>
std::array<Vector<N, T>, N> generate_edges(const T edge_min_length, const T edge_max_length, Engine& engine)
{
        namespace impl = parallelotope_points_implementation;

        ASSERT(edge_min_length > 0 && edge_min_length <= edge_max_length);

        std::uniform_real_distribution<T> urd(-edge_max_length, edge_max_length);
        std::array<Vector<N, T>, N> edges;

        do
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        for (std::size_t j = 0; j < N; ++j)
                        {
                                edges[i][j] = urd(engine);
                        }
                }

        } while (!impl::test_edges(edge_min_length, edge_max_length, edges));

        return edges;
}

template <std::size_t N, typename T, typename Engine>
std::array<T, N> generate_aa_edges(const T edge_min_length, const T edge_max_length, Engine& engine)
{
        ASSERT(edge_min_length > 0 && edge_min_length <= edge_max_length);

        std::uniform_real_distribution<T> urd(edge_min_length, edge_max_length);
        std::array<T, N> edges;
        for (std::size_t i = 0; i < N; ++i)
        {
                edges[i] = urd(engine);
        }
        return edges;
}

template <std::size_t N, typename T, typename Engine>
Vector<N, T> generate_org(const T org_size, Engine& engine)
{
        ASSERT(org_size >= 0);

        std::uniform_real_distribution<T> urd(-org_size, org_size);
        Vector<N, T> org;
        for (std::size_t i = 0; i < N; ++i)
        {
                org[i] = urd(engine);
        }
        return org;
}
}
