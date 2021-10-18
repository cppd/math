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

#include <src/geometry/shapes/parallelotope_volume.h>
#include <src/numerical/vec.h>

#include <array>

namespace ns::sampling
{
template <std::size_t N, typename T, std::size_t M>
Vector<N, T> uniform_in_parallelotope(const std::array<Vector<N, T>, M>& vectors, const Vector<M, T>& samples)
{
        static_assert(N > 0 && M > 0 && M <= N);

        Vector<N, T> res = samples[0] * vectors[0];
        for (std::size_t i = 1; i < M; ++i)
        {
                res.multiply_add(samples[i], vectors[i]);
        }
        return res;
}

template <std::size_t N, typename T, std::size_t M>
T uniform_in_parallelotope_pdf(const std::array<Vector<N, T>, M>& vectors)
{
        return 1 / geometry::parallelotope_volume(vectors);
}
}