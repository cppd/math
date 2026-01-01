/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::geometry::spatial
{
namespace simplex_edges_implementation
{
template <std::size_t N>
inline constexpr int EDGE_COUNT = BINOMIAL<N, 2>;
}

template <std::size_t N, typename T, std::size_t M>
std::array<std::array<numerical::Vector<N, T>, 2>, simplex_edges_implementation::EDGE_COUNT<M>> simplex_edges(
        const std::array<numerical::Vector<N, T>, M>& vertices)
{
        static_assert(M > 0 && M <= N);
        static_assert(N <= 3);

        static constexpr std::size_t EDGE_COUNT = simplex_edges_implementation::EDGE_COUNT<M>;

        std::array<std::array<numerical::Vector<N, T>, 2>, EDGE_COUNT> res;
        std::size_t n = 0;
        for (std::size_t i = 0; i < M - 1; ++i)
        {
                for (std::size_t j = i + 1; j < M; ++j)
                {
                        res[n++] = {vertices[i], vertices[j] - vertices[i]};
                }
        }
        ASSERT(n == EDGE_COUNT);
        return res;
}
}
