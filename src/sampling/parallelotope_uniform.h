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

#include <src/geometry/shapes/parallelotope_volume.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <random>

namespace ns::sampling
{
template <std::size_t N, typename T, std::size_t M>
numerical::Vector<N, T> uniform_in_parallelotope(
        const std::array<numerical::Vector<N, T>, M>& vectors,
        const numerical::Vector<M, T>& samples)
{
        static_assert(N > 0 && M > 0 && M <= N);

        numerical::Vector<N, T> res = samples[0] * vectors[0];
        for (std::size_t i = 1; i < M; ++i)
        {
                res.multiply_add(samples[i], vectors[i]);
        }
        return res;
}

template <std::size_t N, typename T, std::size_t M, std::uniform_random_bit_generator RandomEngine>
numerical::Vector<N, T> uniform_in_parallelotope(
        RandomEngine& engine,
        const std::array<numerical::Vector<N, T>, M>& vectors)
{
        static_assert(N > 0 && M > 0 && M <= N);

        std::uniform_real_distribution<T> urd(0, 1);
        numerical::Vector<N, T> res = vectors[0] * urd(engine);
        for (std::size_t i = 1; i < M; ++i)
        {
                res.multiply_add(vectors[i], urd(engine));
        }
        return res;
}

template <std::size_t N, typename T, std::size_t M>
T uniform_in_parallelotope_pdf(const std::array<numerical::Vector<N, T>, M>& vectors)
{
        return 1 / geometry::shapes::parallelotope_volume(vectors);
}
}
