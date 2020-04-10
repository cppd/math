/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/numerical/vec.h>

#include <array>

namespace painter
{
template <typename HyperplaneParallelotope>
class HyperplaneParallelotopeAlgorithm final
{
        static constexpr size_t N = HyperplaneParallelotope::DIMENSION;
        using T = typename HyperplaneParallelotope::DataType;

        static_assert(N <= 30);

        static constexpr int VERTEX_COUNT = 1 << (N - 1);

        template <int n, typename F>
        static void vertices_impl(const HyperplaneParallelotope& p, const Vector<N, T>& org, const F& f)
        {
                if constexpr (n >= 0)
                {
                        vertices_impl<n - 1>(p, org, f);
                        vertices_impl<n - 1>(p, org + p.e(n), f);
                }
                else
                {
                        f(org);
                }
        }

        template <typename F>
        static void vertices(const HyperplaneParallelotope& p, const F& f)
        {
                constexpr int last_index = N - 2;

                // Смещаться по каждому измерению для перехода к другой вершине.

                vertices_impl<last_index>(p, p.org(), f);
        }

public:
        using Vertices = std::array<Vector<N, T>, VERTEX_COUNT>;

        static Vertices vertices(const HyperplaneParallelotope& r)
        {
                Vertices result;

                unsigned vertex_count = 0;

                vertices(r, [&vertex_count, &result](const Vector<N, T>& org) {
                        ASSERT(vertex_count < result.size());
                        result[vertex_count++] = org;
                });

                ASSERT(vertex_count == result.size());

                return result;
        }
};

template <typename HyperplaneParallelotope>
typename HyperplaneParallelotopeAlgorithm<HyperplaneParallelotope>::Vertices hyperplane_parallelotope_vertices(
        const HyperplaneParallelotope& r)
{
        return HyperplaneParallelotopeAlgorithm<HyperplaneParallelotope>::vertices(r);
}
}
