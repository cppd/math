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

#include "vec.h"

#include <src/com/arrays.h>

#include <array>

namespace ns::numerical
{
// Cofactor expansion
template <std::size_t N_V, std::size_t N_H, typename T, std::size_t SIZE>
constexpr T determinant(
        const std::array<Vector<N_H, T>, N_V>& vectors,
        const std::array<unsigned char, SIZE>& v_map,
        const std::array<unsigned char, SIZE>& h_map)
{
        static_assert(N_V >= SIZE);
        static_assert(N_H >= SIZE);
        static_assert(SIZE > 0);

        if constexpr (SIZE == 1)
        {
                return vectors[v_map[0]][h_map[0]];
        }
        else if constexpr (SIZE == 2)
        {
                return vectors[v_map[0]][h_map[0]] * vectors[v_map[1]][h_map[1]]
                       - vectors[v_map[0]][h_map[1]] * vectors[v_map[1]][h_map[0]];
        }
        else if constexpr (SIZE == 3)
        {
                const T& v00 = vectors[v_map[0]][h_map[0]];
                const T& v01 = vectors[v_map[0]][h_map[1]];
                const T& v02 = vectors[v_map[0]][h_map[2]];
                const T& v10 = vectors[v_map[1]][h_map[0]];
                const T& v11 = vectors[v_map[1]][h_map[1]];
                const T& v12 = vectors[v_map[1]][h_map[2]];
                const T& v20 = vectors[v_map[2]][h_map[0]];
                const T& v21 = vectors[v_map[2]][h_map[1]];
                const T& v22 = vectors[v_map[2]][h_map[2]];

                const T& d0 = v00 * (v11 * v22 - v12 * v21);
                const T& d1 = v01 * (v10 * v22 - v12 * v20);
                const T& d2 = v02 * (v10 * v21 - v11 * v20);

                return d0 - d1 + d2;
        }
        else
        {
                T det = 0;

                const unsigned char row = v_map[0];
                const std::array<unsigned char, SIZE - 1> map = del_elem(v_map, 0);

                for (std::size_t i = 0; i < SIZE; ++i)
                {
                        const T& entry = vectors[row][h_map[i]];
                        const T& minor = determinant(vectors, map, del_elem(h_map, i));

                        if (i & 1)
                        {
                                det -= entry * minor;
                        }
                        else
                        {
                                det += entry * minor;
                        }
                }

                return det;
        }
}

template <std::size_t N, typename T>
constexpr T determinant(const std::array<Vector<N, T>, N>& vectors)
{
        return determinant(vectors, sequence_uchar_array<N>, sequence_uchar_array<N>);
}
}
