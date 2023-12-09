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

#include "parallelotope_volume.h"

#include <src/com/combinatorics.h>
#include <src/numerical/vector.h>

#include <array>

namespace ns::geometry::shapes
{
template <std::size_t N, typename T, std::size_t M>
T simplex_volume(const std::array<Vector<N, T>, M>& vertices)
{
        static_assert(N > 0 && M >= 2 && M <= N + 1);

        std::array<Vector<N, T>, M - 1> vectors;
        for (std::size_t i = 0; i < M - 1; ++i)
        {
                vectors[i] = vertices[i + 1] - vertices[0];
        }

        return parallelotope_volume(vectors) * (T{1} / FACTORIAL<M - 1>);
}
}
