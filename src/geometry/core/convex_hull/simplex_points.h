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

#pragma once

#include <src/com/combinatorics.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/conversion.h>
#include <src/numerical/determinant.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace ns::geometry::core::convex_hull
{
namespace simplex_points_implementation
{
// The T range is for determinants only, not for Gram matrix
template <int COUNT, std::size_t N, typename T>
bool linearly_independent(const std::array<numerical::Vector<N, T>, N>& vectors)
{
        static_assert(Integral<T>);
        static_assert(N > 1);
        static_assert(COUNT > 0 && COUNT <= N);

        return std::any_of(
                COMBINATIONS<N, COUNT>.cbegin(), COMBINATIONS<N, COUNT>.cend(),
                [&vectors](const std::array<unsigned char, COUNT>& h_map)
                {
                        return numerical::determinant(vectors, SEQUENCE_UCHAR_ARRAY<COUNT>, h_map) != 0;
                });
}

template <unsigned SIMPLEX_I, std::size_t N, typename SourceType, typename ComputeType>
void find_simplex_points(
        const std::vector<numerical::Vector<N, SourceType>>& points,
        std::array<int, N + 1>* const simplex_points,
        std::array<numerical::Vector<N, ComputeType>, N>* const simplex_vectors,
        unsigned point_i)
{
        static_assert(N > 1);
        static_assert(SIMPLEX_I <= N);

        const numerical::Vector<N, SourceType>& p = points[(*simplex_points)[0]];
        for (; point_i < points.size(); ++point_i)
        {
                numerical::set_vector(&(*simplex_vectors)[SIMPLEX_I - 1], points[point_i], p);

                if (linearly_independent<SIMPLEX_I>(*simplex_vectors))
                {
                        break;
                }
        }

        if (point_i == points.size())
        {
                error("point " + to_string(SIMPLEX_I + 1) + " of " + to_string(N) + "-simplex not found");
        }

        (*simplex_points)[SIMPLEX_I] = point_i;

        if constexpr (SIMPLEX_I + 1 < N + 1)
        {
                find_simplex_points<SIMPLEX_I + 1>(points, simplex_points, simplex_vectors, point_i + 1);
        }
}
}

template <std::size_t N, typename SourceType, typename ComputeType>
std::array<int, N + 1> find_simplex_points(const std::vector<numerical::Vector<N, SourceType>>& points)
{
        static_assert(N > 1);

        if (points.empty())
        {
                error("0-simplex not found");
        }

        std::array<numerical::Vector<N, ComputeType>, N> simplex_vectors;

        std::array<int, N + 1> simplex_points;
        simplex_points[0] = 0;

        simplex_points_implementation::find_simplex_points<1>(points, &simplex_points, &simplex_vectors, 1);

        return simplex_points;
}
}
