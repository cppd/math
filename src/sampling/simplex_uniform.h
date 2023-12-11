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

#include <src/com/sort.h>
#include <src/com/type/limit.h>
#include <src/geometry/shapes/simplex_volume.h>
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>

namespace ns::sampling
{
namespace simplex_implementation
{
template <std::size_t N, typename T, std::size_t M, typename RandomEngine>
Vector<N, T> uniform_in_simplex_1(RandomEngine& engine, const std::array<Vector<N, T>, M>& vertices)
{
        std::array<T, M - 1> random_points;
        std::uniform_real_distribution<T> urd(0, 1 + Limits<T>::epsilon());
        for (std::size_t i = 0; i < M - 1; ++i)
        {
                do
                {
                        random_points[i] = urd(engine);
                } while (random_points[i] > 1);
        }
        sort(random_points);

        Vector<N, T> res = vertices[0] * random_points[0];
        for (std::size_t i = 1; i < M - 1; ++i)
        {
                res.multiply_add(vertices[i], random_points[i] - random_points[i - 1]);
        }
        res.multiply_add(vertices[M - 1], 1 - random_points.back());

        return res;
}

template <std::size_t N, typename T, std::size_t M, typename RandomEngine>
Vector<N, T> uniform_in_simplex_2(RandomEngine& engine, const std::array<Vector<N, T>, M>& vertices)
{
        std::array<T, M> coordinates;
        std::uniform_real_distribution<T> urd(-1, 0);
        T sum;
        do
        {
                sum = 0;
                for (std::size_t i = 0; i < M; ++i)
                {
                        const T c = -std::log(-urd(engine));
                        coordinates[i] = c;
                        sum += c;
                }
        } while (!(std::isfinite(sum) && sum > 0));

        Vector<N, T> res = vertices[0] * (coordinates[0] / sum);
        for (std::size_t i = 1; i < M; ++i)
        {
                res.multiply_add(vertices[i], coordinates[i] / sum);
        }
        return res;
}
}

template <std::size_t N, typename T, std::size_t M, typename RandomEngine>
Vector<N, T> uniform_in_simplex(RandomEngine& engine, const std::array<Vector<N, T>, M>& vertices)
{
        static_assert(N > 0 && M >= 2 && M <= N + 1);

        switch (1)
        {
        case 1:
                return simplex_implementation::uniform_in_simplex_1(engine, vertices);
        case 2:
                return simplex_implementation::uniform_in_simplex_2(engine, vertices);
        }
}

template <std::size_t N, typename T, std::size_t M>
T uniform_in_simplex_pdf(const std::array<Vector<N, T>, M>& vertices)
{
        return 1 / geometry::shapes::simplex_volume(vertices);
}
}
