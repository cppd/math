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

#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::geometry::spatial
{
namespace parallelotope_vertices_implementation
{
template <std::size_t N>
inline constexpr int VERTEX_COUNT = 1 << N;

template <int INDEX, std::size_t N, std::size_t M, typename T, typename F>
void vertices(const Vector<N, T>& p, const std::array<Vector<N, T>, M>& vectors, const F& f)
{
        if constexpr (INDEX >= 0)
        {
                vertices<INDEX - 1>(p, vectors, f);
                vertices<INDEX - 1>(p + vectors[INDEX], vectors, f);
        }
        else
        {
                f(p);
        }
}

template <int INDEX, std::size_t N, typename T, typename F>
void vertices(const Vector<N, T>& min, const Vector<N, T>& max, Vector<N, T>* const p, const F& f)
{
        if constexpr (INDEX >= 0)
        {
                (*p)[INDEX] = min[INDEX];
                vertices<INDEX - 1>(min, max, p, f);
                (*p)[INDEX] = max[INDEX];
                vertices<INDEX - 1>(min, max, p, f);
        }
        else
        {
                f();
        }
}
}

template <std::size_t N, typename T, std::size_t M>
std::array<Vector<N, T>, parallelotope_vertices_implementation::VERTEX_COUNT<M>> parallelotope_vertices(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, M>& vectors)
{
        static_assert(N > 0);
        static_assert(M > 0 && M <= N);

        namespace impl = parallelotope_vertices_implementation;

        static constexpr std::size_t VERTEX_COUNT = impl::VERTEX_COUNT<M>;

        std::array<Vector<N, T>, VERTEX_COUNT> res;

        unsigned count = 0;

        const auto f = [&count, &res](const Vector<N, T>& p)
        {
                ASSERT(count < res.size());
                res[count++] = p;
        };

        impl::vertices<M - 1>(org, vectors, f);

        ASSERT(count == res.size());

        return res;
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, parallelotope_vertices_implementation::VERTEX_COUNT<N>> parallelotope_vertices(
        const Vector<N, T>& min,
        const Vector<N, T>& max)
{
        static_assert(N > 0);

        namespace impl = parallelotope_vertices_implementation;

        static constexpr std::size_t VERTEX_COUNT = impl::VERTEX_COUNT<N>;

        std::array<Vector<N, T>, VERTEX_COUNT> res;

        unsigned count = 0;
        Vector<N, T> p;

        const auto f = [&]
        {
                ASSERT(count < res.size());
                res[count++] = p;
        };

        impl::vertices<N - 1>(min, max, &p, f);

        ASSERT(count == res.size());

        return res;
}
}
