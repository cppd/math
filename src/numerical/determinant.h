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

#include <src/com/arrays.h>
#include <src/com/combinatorics.h>

#include <array>

namespace ns::numerical
{
namespace determinant_implementation
{
template <std::size_t N_V, std::size_t N_H, typename T, std::size_t DET_SIZE>
constexpr T determinant_by_cofactor_expansion(
        const std::array<Vector<N_H, T>, N_V>& vectors,
        const std::array<unsigned char, DET_SIZE>& v_map,
        const std::array<unsigned char, DET_SIZE>& h_map)
{
        static_assert(N_V >= DET_SIZE);
        static_assert(N_H >= DET_SIZE);
        static_assert(DET_SIZE > 0);

        // Надо выделить v_map.size() == h_map.size() векторов по вертикали и горизонтали,
        // определяя коэффициенты по v_map и h_map.
        // Например, строки с номерами v_map 0 и 3, в каждой строке надо взять элементы
        // h_map с номерами 1 и 4. Получается матрица 2x2.
        //           h_map
        //        x ~ x x ~ x
        //        x x x x x x
        // v_map  x x x x x x
        //        x ~ x x ~ x
        //        x x x x x x

        if constexpr (DET_SIZE == 1)
        {
                return vectors[v_map[0]][h_map[0]];
        }
        if constexpr (DET_SIZE == 2)
        {
                return vectors[v_map[0]][h_map[0]] * vectors[v_map[1]][h_map[1]]
                       - vectors[v_map[0]][h_map[1]] * vectors[v_map[1]][h_map[0]];
        }
        if constexpr (DET_SIZE == 3)
        {
                T v00 = vectors[v_map[0]][h_map[0]];
                T v01 = vectors[v_map[0]][h_map[1]];
                T v02 = vectors[v_map[0]][h_map[2]];
                T v10 = vectors[v_map[1]][h_map[0]];
                T v11 = vectors[v_map[1]][h_map[1]];
                T v12 = vectors[v_map[1]][h_map[2]];
                T v20 = vectors[v_map[2]][h_map[0]];
                T v21 = vectors[v_map[2]][h_map[1]];
                T v22 = vectors[v_map[2]][h_map[2]];

                T d0 = v00 * (v11 * v22 - v12 * v21);
                T d1 = v01 * (v10 * v22 - v12 * v20);
                T d2 = v02 * (v10 * v21 - v11 * v20);

                return d0 - d1 + d2;
        }
        if constexpr (DET_SIZE >= 4)
        {
                T det = 0;

                for (unsigned i = 0; i < DET_SIZE; ++i)
                {
                        T entry = vectors[v_map[0]][h_map[i]];
                        T minor = determinant_by_cofactor_expansion(vectors, del_elem(v_map, 0), del_elem(h_map, i));

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
}

template <std::size_t N_V, std::size_t N_H, typename T, std::size_t DET_SIZE>
constexpr T determinant(
        const std::array<Vector<N_H, T>, N_V>& vectors,
        const std::array<unsigned char, DET_SIZE>& v_map,
        const std::array<unsigned char, DET_SIZE>& h_map)
{
        return determinant_implementation::determinant_by_cofactor_expansion(vectors, v_map, h_map);
}

// Тип данных передаваемых векторов и диапазон значений рассчитаны только на определители
// из этих чисел, поэтому нельзя использовать скалярные произведения, матрицы Грама и т.п.
template <int COUNT, std::size_t N, typename T>
bool linearly_independent(const std::array<Vector<N, T>, N>& vectors)
{
        static_assert(is_integral<T>);
        static_assert(N > 1);
        static_assert(COUNT > 0);
        static_assert(COUNT <= N);

        // Перебор всех вариантов подвекторов размера COUNT,
        // создавая квадратные матрицы размером COUNT.
        for (const std::array<unsigned char, COUNT>& h_map : combinations<N, COUNT>())
        {
                if (determinant(vectors, sequence_uchar_array<COUNT>, h_map) != 0)
                {
                        return true;
                }
        }

        return false;
}
}
