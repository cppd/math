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

#include "matrix.h"
#include "vector.h"

#include <array>
#include <cstddef>

namespace ns::numerical
{
template <std::size_t N, std::size_t M, typename T>
Matrix<M, M, T> gram_matrix(const std::array<Vector<N, T>, M>& vectors)
{
        static_assert(N > 0 && M > 0 && M <= N);

        Matrix<M, M, T> res;

        for (std::size_t r = 0; r < M; ++r)
        {
                res[r, r] = dot(vectors[r], vectors[r]);
                for (std::size_t c = r + 1; c < M; ++c)
                {
                        res[r, c] = dot(vectors[r], vectors[c]);
                        res[c, r] = res[r, c];
                }
        }

        return res;
}
}
