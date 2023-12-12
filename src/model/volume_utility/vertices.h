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

#include "../volume.h"

#include <src/com/error.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::model::volume
{
namespace vertices_implementation
{
template <int I, typename F, std::size_t N, typename T>
void vertices_impl(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors, const F& f)
{
        if constexpr (I >= 0)
        {
                vertices_impl<I - 1>(org, vectors, f);
                vertices_impl<I - 1>(org + vectors[I], vectors, f);
        }
        else
        {
                f(org);
        }
}

template <typename F, std::size_t N, typename T>
void vertices(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors, const F& f)
{
        vertices_impl<N - 1>(org, vectors, f);
}
}

template <std::size_t N>
std::array<Vector<N, double>, (1 << N)> vertices(const Volume<N>& volume)
{
        namespace impl = vertices_implementation;

        const numerical::transform::MatrixVectorMultiplier transform(volume.matrix);

        const Vector<N, double> org = transform(Vector<N, double>(0));

        std::array<Vector<N, double>, N> vectors;
        for (unsigned i = 0; i < N; ++i)
        {
                vectors[i] = Vector<N, double>(0);
                vectors[i][i] = 1;
                vectors[i] = transform(vectors[i]);
        }

        std::array<Vector<N, double>, (1 << N)> res;
        unsigned vertex_count = 0;

        impl::vertices(
                org, vectors,
                [&vertex_count, &res](const Vector<N, double>& p)
                {
                        ASSERT(vertex_count < res.size());
                        res[vertex_count++] = p;
                });

        ASSERT(vertex_count == res.size());

        return res;
}
}
