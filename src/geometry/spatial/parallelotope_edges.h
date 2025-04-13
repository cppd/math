/*
Copyright (C) 2017-2025 Topological Manifold

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
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::geometry::spatial
{
namespace parallelotope_edges_implementation
{
// Vertex count 2 ^ N multiplied by vertex dimension
// count N and divided by 2 for uniqueness
// ((2 ^ N) * N) / 2 = (2 ^ (N - 1)) * N
template <std::size_t N>
inline constexpr int EDGE_COUNT = (1 << (N - 1)) * N;

template <int INDEX, std::size_t N, std::size_t M, typename T, typename F>
void edges(
        const numerical::Vector<N, T>& p,
        const std::array<numerical::Vector<N, T>, M>& vectors,
        std::array<bool, M>* const dimensions,
        const F& f)
{
        if constexpr (INDEX >= 0)
        {
                (*dimensions)[INDEX] = true;
                edges<INDEX - 1>(p, vectors, dimensions, f);

                (*dimensions)[INDEX] = false;
                edges<INDEX - 1>(p + vectors[INDEX], vectors, dimensions, f);
        }
        else
        {
                f(p);
        }
}

template <int INDEX, std::size_t N, typename T, typename F>
void edges(
        const numerical::Vector<N, T>& min,
        const numerical::Vector<N, T>& max,
        numerical::Vector<N, T>* const p,
        std::array<bool, N>* const dimensions,
        const F& f)
{
        if constexpr (INDEX >= 0)
        {
                (*dimensions)[INDEX] = true;
                (*p)[INDEX] = min[INDEX];
                edges<INDEX - 1>(min, max, p, dimensions, f);

                (*dimensions)[INDEX] = false;
                (*p)[INDEX] = max[INDEX];
                edges<INDEX - 1>(min, max, p, dimensions, f);
        }
        else
        {
                f();
        }
}
}

template <std::size_t N, typename T, std::size_t M>
std::array<std::array<numerical::Vector<N, T>, 2>, parallelotope_edges_implementation::EDGE_COUNT<M>>
        parallelotope_edges(const numerical::Vector<N, T>& org, const std::array<numerical::Vector<N, T>, M>& vectors)
{
        static_assert(N > 0 && N <= 3);
        static_assert(M > 0 && M <= N);

        namespace impl = parallelotope_edges_implementation;

        static constexpr std::size_t EDGE_COUNT = impl::EDGE_COUNT<M>;

        std::array<std::array<numerical::Vector<N, T>, 2>, EDGE_COUNT> res;

        unsigned count = 0;
        std::array<bool, M> dimensions;

        const auto f = [&](const numerical::Vector<N, T>& p)
        {
                for (std::size_t i = 0; i < M; ++i)
                {
                        if (dimensions[i])
                        {
                                ASSERT(count < res.size());
                                res[count][0] = p;
                                res[count][1] = vectors[i];
                                ++count;
                        }
                }
        };

        impl::edges<M - 1>(org, vectors, &dimensions, f);
        ASSERT(count == res.size());

        return res;
}

template <std::size_t N, typename T>
std::array<std::array<numerical::Vector<N, T>, 2>, parallelotope_edges_implementation::EDGE_COUNT<N>>
        parallelotope_edges(const numerical::Vector<N, T>& min, const numerical::Vector<N, T>& max)
{
        static_assert(N > 0 && N <= 3);

        namespace impl = parallelotope_edges_implementation;

        static constexpr std::size_t EDGE_COUNT = impl::EDGE_COUNT<N>;

        const numerical::Vector<N, T> diagonal = max - min;

        std::array<std::array<numerical::Vector<N, T>, 2>, EDGE_COUNT> res;

        unsigned count = 0;
        numerical::Vector<N, T> p;
        std::array<bool, N> dimensions;

        const auto f = [&]
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (dimensions[i])
                        {
                                ASSERT(count < res.size());
                                res[count][0] = p;
                                res[count][1] = numerical::Vector<N, T>(0);
                                res[count][1][i] = diagonal[i];
                                ++count;
                        }
                }
        };

        impl::edges<N - 1>(min, max, &p, &dimensions, f);
        ASSERT(count == res.size());

        return res;
}
}
