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

#include "vec.h"

#include <array>

template <size_t ArraySize, typename T, size_t N>
void min_max_vector(const std::array<Vector<N, T>, ArraySize>& vectors, Vector<N, T>* min, Vector<N, T>* max)
{
        static_assert(ArraySize > 0);

        *min = vectors[0];
        *max = vectors[0];

        for (size_t i = 1; i < ArraySize; ++i)
        {
                *min = min_vector(vectors[i], *min);
                *max = max_vector(vectors[i], *max);
        }
}
