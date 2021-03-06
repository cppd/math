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

#include "../volume.h"

#include <src/numerical/transform.h>
#include <src/numerical/vec.h>

#include <array>

namespace ns::volume
{
namespace vertices_implementation
{
template <int n, typename F, std::size_t N, typename T>
void vertices_impl(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors, const F& f)
{
        if constexpr (n >= 0)
        {
                vertices_impl<n - 1>(org, vectors, f);
                vertices_impl<n - 1>(org + vectors[n], vectors, f);
        }
        else
        {
                f(org);
        }
}
template <typename F, std::size_t N, typename T>
void vertices(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors, const F& f)
{
        constexpr int last_index = N - 1;

        // Смещаться по каждому измерению для перехода к другой вершине.

        vertices_impl<last_index>(org, vectors, f);
}
}

template <std::size_t N>
std::array<Vector<N, double>, (1 << N)> vertices(const Volume<N>& volume)
{
        namespace impl = vertices_implementation;

        const matrix::MatrixVectorMultiplier transform(volume.matrix);

        Vector<N, double> org = transform(Vector<N, double>(0));

        std::array<Vector<N, double>, N> vectors;
        for (unsigned n = 0; n < N; ++n)
        {
                vectors[n] = Vector<N, double>(0);
                vectors[n][n] = 1;
                vectors[n] = transform(vectors[n]);
        }

        std::array<Vector<N, double>, (1 << N)> result;
        unsigned vertex_count = 0;

        impl::vertices(
                org, vectors,
                [&vertex_count, &result](const Vector<N, double>& p)
                {
                        ASSERT(vertex_count < result.size());
                        result[vertex_count++] = p;
                });

        ASSERT(vertex_count == result.size());

        return result;
}
}
