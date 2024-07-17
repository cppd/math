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

#include <src/com/exponent.h>
#include <src/numerical/complement.h>
#include <src/numerical/determinant.h>
#include <src/numerical/gram.h>
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <cstddef>

namespace ns::geometry::shapes
{
template <std::size_t N, typename T, std::size_t M>
T parallelotope_volume(const std::array<numerical::Vector<N, T>, M>& vectors)
{
        static_assert(N > 0 && M > 0 && M <= N);

        if constexpr (N == M)
        {
                return std::abs(numerical::determinant(vectors));
        }
        else if constexpr (N == M + 1)
        {
                return numerical::orthogonal_complement(vectors).norm();
        }
        else
        {
                return sqrt_s(numerical::gram_matrix(vectors).determinant());
        }
}
}
