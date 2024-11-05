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

#include "gauss.h"
#include "vector.h"

#include <src/com/arrays.h>
#include <src/com/type/concept.h>

#include <array>
#include <cstddef>

namespace ns::numerical
{
namespace determinant_implementation
{
template <std::size_t N_V, std::size_t N_H, typename T, std::size_t SIZE>
constexpr T determinant_cofactor_expansion(
        const std::array<Vector<N_H, T>, N_V>& vectors,
        const std::array<unsigned char, SIZE>& v_map,
        const std::array<unsigned char, SIZE>& h_map)
{
        static_assert(N_V >= SIZE);
        static_assert(N_H >= SIZE);

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
        else if constexpr (SIZE >= 4)
        {
                T det = 0;

                const unsigned char row = v_map[0];
                const std::array<unsigned char, SIZE - 1> map = del_elem(v_map, 0);

                for (std::size_t i = 0; i < SIZE; ++i)
                {
                        const T& entry = vectors[row][h_map[i]];
                        const T& minor = determinant_cofactor_expansion(vectors, map, del_elem(h_map, i));

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
        else
        {
                static_assert(false);
        }
}
}

template <std::size_t N_V, std::size_t N_H, typename T, std::size_t SIZE>
constexpr T determinant(
        const std::array<Vector<N_H, T>, N_V>& vectors,
        const std::array<unsigned char, SIZE>& v_map,
        const std::array<unsigned char, SIZE>& h_map)
{
        static_assert(Signed<T>);
        static_assert(Integral<T>);

        return determinant_implementation::determinant_cofactor_expansion(vectors, v_map, h_map);
}

template <std::size_t N, typename T>
constexpr T determinant(const std::array<Vector<N, T>, N - 1>& vectors, const std::size_t excluded_column)
{
        static_assert(Signed<T>);
        static_assert(Integral<T> || FloatingPoint<T>);

        if constexpr (Integral<T>)
        {
                return determinant_implementation::determinant_cofactor_expansion(
                        vectors, SEQUENCE_UCHAR_ARRAY<N - 1>, del_elem(SEQUENCE_UCHAR_ARRAY<N>, excluded_column));
        }
        if constexpr (FloatingPoint<T>)
        {
                if constexpr (N <= 6)
                {
                        return determinant_implementation::determinant_cofactor_expansion(
                                vectors, SEQUENCE_UCHAR_ARRAY<N - 1>,
                                del_elem(SEQUENCE_UCHAR_ARRAY<N>, excluded_column));
                }
                else
                {
                        return determinant_gauss(vectors, excluded_column);
                }
        }
}

template <std::size_t N, typename T>
constexpr T determinant(const std::array<Vector<N, T>, N>& vectors)
{
        static_assert(Signed<T>);
        static_assert(Integral<T> || FloatingPoint<T>);

        if constexpr (Integral<T>)
        {
                return determinant_implementation::determinant_cofactor_expansion(
                        vectors, SEQUENCE_UCHAR_ARRAY<N>, SEQUENCE_UCHAR_ARRAY<N>);
        }
        if constexpr (FloatingPoint<T>)
        {
                if constexpr (N <= 5)
                {
                        return determinant_implementation::determinant_cofactor_expansion(
                                vectors, SEQUENCE_UCHAR_ARRAY<N>, SEQUENCE_UCHAR_ARRAY<N>);
                }
                else
                {
                        return determinant_gauss(vectors);
                }
        }
}
}
