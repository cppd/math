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

#include "../bounding_box.h"

#include <array>
#include <random>

namespace ns::geometry::spatial::test
{
template <std::size_t N, typename T>
BoundingBox<N, T> create_random_bounding_box(std::mt19937_64& engine)
{
        std::uniform_real_distribution<T> urd(-5, 5);
        Vector<N, T> p1;
        Vector<N, T> p2;
        for (std::size_t i = 0; i < N; ++i)
        {
                do
                {
                        p1[i] = urd(engine);
                        p2[i] = urd(engine);
                } while (!(std::abs(p1[i] - p2[i]) >= T(0.5)));
        }
        return {p1, p2};
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N> bounding_box_vectors(const BoundingBox<N, T>& box)
{
        const Vector<N, T> diagonal = box.diagonal();
        std::array<Vector<N, T>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = 0; j < N; ++j)
                {
                        res[i][j] = 0;
                }
                res[i][i] = diagonal[i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, bool> bounding_box_negative_directions(const Vector<N, T>& v)
{
        Vector<N, bool> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = (v[i] < 0);
        }
        return res;
}
}
