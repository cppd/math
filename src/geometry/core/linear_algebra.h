/*
Copyright (C) 2017 Topological Manifold

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

#include "array_elements.h"

#include "com/arrays.h"
#include "com/combinatorics.h"
#include "com/math.h"
#include "com/vec.h"

#include <array>
#include <gmpxx.h>
#include <vector>

namespace LinearAlgebraImplementation
{
template <size_t N>
inline constexpr std::array<unsigned char, N> sequence_array = make_array_sequence<unsigned char, N>();

template <size_t N_V, size_t N_H, typename T, size_t DET_SIZE>
constexpr T determinant_by_cofactor_expansion(const std::array<Vector<N_H, T>, N_V>& vectors,
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
        else if constexpr (DET_SIZE == 2)
        {
                return vectors[v_map[0]][h_map[0]] * vectors[v_map[1]][h_map[1]] -
                       vectors[v_map[0]][h_map[1]] * vectors[v_map[1]][h_map[0]];
        }
        else if constexpr (DET_SIZE == 3)
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
        else
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

template <size_t N_V, size_t N_H, typename T, size_t DET_SIZE>
constexpr T determinant(const std::array<Vector<N_H, T>, N_V>& vectors, const std::array<unsigned char, DET_SIZE>& v_map,
                        const std::array<unsigned char, DET_SIZE>& h_map)
{
        return determinant_by_cofactor_expansion(vectors, v_map, h_map);
}

// GCC 7.2 для функций del_elem выдаёт ошибку
// "sorry, unimplemented: unexpected AST of kind switch_expr".
#if defined(__clang__)
// clang-format off
static_assert
(
        1'868'201'030'776'500
        ==
        determinant<7, 7, __int128, 7>
        (
        {{
        {10,  2,   3,   4,   5,   6,   7},
        { 8, 90,  10,  11,  12,  13,  14},
        {15, 16, 170,  18,  19,  20,  21},
        {22, 23,  24, 250,  26,  27,  28},
        {29, 30,  31,  32, 330,  34,  35},
        {36, 37,  38,  39,  40, 410,  42},
        {43, 44,  45,  46,  47,  48, 490}
        }},
        sequence_array<7>,
        sequence_array<7>
        )
);
// clang-format on
#endif
}

// Тип данных передаваемых векторов и диапазон значений рассчитаны только на определители
// из этих чисел, поэтому нельзя использовать скалярные произведения, матрицы Грама и т.п.
template <int COUNT, size_t N, typename T>
std::enable_if_t<any_integral<T>, bool> linearly_independent(const std::array<Vector<N, T>, N>& vectors)
{
        static_assert(N > 1);
        static_assert(COUNT > 0);
        static_assert(COUNT <= N);

        namespace Impl = LinearAlgebraImplementation;

        // Перебор всех вариантов подвекторов размера COUNT,
        // создавая квадратные матрицы размером COUNT.
        for (const std::array<unsigned char, COUNT>& h_map : get_combinations<N, COUNT>())
        {
                if (Impl::determinant(vectors, Impl::sequence_array<COUNT>, h_map) != 0)
                {
                        return true;
                }
        }

        return false;
}

// Вектор из ортогонального дополнения (n-1)-мерного подпространства
template <size_t N, typename T>
Vector<N, T> ortho_nn(const std::array<Vector<N, T>, N - 1>& vectors)
{
        static_assert(N > 1);

        namespace Impl = LinearAlgebraImplementation;

        // Для расчёта используются N - 1 строк и N столбцов.
        // Для отображения элементов в элементы используются sequence_array,
        // которые заполнены последовательными числами, начиная с 0.

        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                T minor = Impl::determinant(vectors, Impl::sequence_array<N - 1>, del_elem(Impl::sequence_array<N>, i));
                res[i] = (i & 1) ? -minor : minor;
        }

        return res;
}

template <typename T>
Vector<2, T> ortho_nn(const std::array<Vector<2, T>, 1>& v)
{
        return Vector<2, T>(v[0][1], -v[0][0]);
}

template <typename T>
Vector<3, T> ortho_nn(const std::array<Vector<3, T>, 2>& v)
{
        Vector<3, T> res;

        // clang-format off

        res[0] = +(v[0][1] * v[1][2] - v[0][2] * v[1][1]);
        res[1] = -(v[0][0] * v[1][2] - v[0][2] * v[1][0]);
        res[2] = +(v[0][0] * v[1][1] - v[0][1] * v[1][0]);

        // clang-format on

        return res;
}

template <typename T>
Vector<4, T> ortho_nn(const std::array<Vector<4, T>, 3>& v)
{
        Vector<4, T> res;

        // clang-format off

        res[0] = + v[0][1] * (v[1][2] * v[2][3] - v[1][3] * v[2][2])
                 - v[0][2] * (v[1][1] * v[2][3] - v[1][3] * v[2][1])
                 + v[0][3] * (v[1][1] * v[2][2] - v[1][2] * v[2][1]);

        res[1] = - v[0][0] * (v[1][2] * v[2][3] - v[1][3] * v[2][2])
                 + v[0][2] * (v[1][0] * v[2][3] - v[1][3] * v[2][0])
                 - v[0][3] * (v[1][0] * v[2][2] - v[1][2] * v[2][0]);

        res[2] = + v[0][0] * (v[1][1] * v[2][3] - v[1][3] * v[2][1])
                 - v[0][1] * (v[1][0] * v[2][3] - v[1][3] * v[2][0])
                 + v[0][3] * (v[1][0] * v[2][1] - v[1][1] * v[2][0]);

        res[3] = - v[0][0] * (v[1][1] * v[2][2] - v[1][2] * v[2][1])
                 + v[0][1] * (v[1][0] * v[2][2] - v[1][2] * v[2][0])
                 - v[0][2] * (v[1][0] * v[2][1] - v[1][1] * v[2][0]);

        // clang-format on

        return res;
}

#if 0
inline void add_mul_ortho(mpz_class* r, const mpz_class& a, const mpz_class& b, const mpz_class& c, const mpz_class& d,
                          const mpz_class& e)
{
        mpz_mul(r->get_mpz_t(), b.get_mpz_t(), c.get_mpz_t());
        mpz_submul(r->get_mpz_t(), d.get_mpz_t(), e.get_mpz_t());
        mpz_mul(r->get_mpz_t(), a.get_mpz_t(), r->get_mpz_t());
}

inline void to_res_ortho(mpz_class* res, const mpz_class& a, const mpz_class& b, const mpz_class& c)
{
        *res = a;
        mpz_sub(res->get_mpz_t(), res->get_mpz_t(), b.get_mpz_t());
        mpz_add(res->get_mpz_t(), res->get_mpz_t(), c.get_mpz_t());
}

inline Vector<4, mpz_class> ortho_nn(const std::array<Vector<4, mpz_class>, 3>& v)
{
        thread_local Vector<4, mpz_class> res;

        thread_local mpz_class x1;
        thread_local mpz_class x2;
        thread_local mpz_class x3;

        add_mul_ortho(&x1, v[0][1], v[1][2], v[2][3], v[1][3], v[2][2]);
        add_mul_ortho(&x2, v[0][2], v[1][1], v[2][3], v[1][3], v[2][1]);
        add_mul_ortho(&x3, v[0][3], v[1][1], v[2][2], v[1][2], v[2][1]);
        to_res_ortho(&res[0], x1, x2, x3);

        add_mul_ortho(&x1, v[0][0], v[1][2], v[2][3], v[1][3], v[2][2]);
        add_mul_ortho(&x2, v[0][2], v[1][0], v[2][3], v[1][3], v[2][0]);
        add_mul_ortho(&x3, v[0][3], v[1][0], v[2][2], v[1][2], v[2][0]);
        to_res_ortho(&res[1], x1, x2, x3);

        add_mul_ortho(&x1, v[0][0], v[1][1], v[2][3], v[1][3], v[2][1]);
        add_mul_ortho(&x2, v[0][1], v[1][0], v[2][3], v[1][3], v[2][0]);
        add_mul_ortho(&x3, v[0][3], v[1][0], v[2][1], v[1][1], v[2][0]);
        to_res_ortho(&res[2], x1, x2, x3);

        add_mul_ortho(&x1, v[0][0], v[1][1], v[2][2], v[1][2], v[2][1]);
        add_mul_ortho(&x2, v[0][1], v[1][0], v[2][2], v[1][2], v[2][0]);
        add_mul_ortho(&x3, v[0][2], v[1][0], v[2][1], v[1][1], v[2][0]);
        to_res_ortho(&res[3], x1, x2, x3);

        mpz_neg(res[1].get_mpz_t(), res[1].get_mpz_t());
        mpz_neg(res[3].get_mpz_t(), res[3].get_mpz_t());

        return res;
}
#endif

template <size_t N, typename T, typename ResultType>
void minus(Vector<N, ResultType>* result, const Vector<N, T>& a, const Vector<N, T>& b)
{
        for (unsigned i = 0; i < N; ++i)
        {
                (*result)[i] = a[i] - b[i];
        }
}
template <size_t N, typename T>
void minus(Vector<N, mpz_class>* result, const Vector<N, T>& a, const Vector<N, T>& b)
{
        for (unsigned i = 0; i < N; ++i)
        {
                mpz_from_any(&(*result)[i], a[i] - b[i]);
        }
}
template <size_t N>
void minus(Vector<N, mpz_class>* result, const Vector<N, mpz_class>& a, const Vector<N, mpz_class>& b)
{
        for (unsigned i = 0; i < N; ++i)
        {
                mpz_sub((*result)[i]->get_mpz_t(), a[i].get_mpz_t(), b[i].get_mpz_t());
        }
}

// Вектор из ортогонального дополнения (n-1)-мерного пространства, определяемого n точками
template <size_t N, typename T, typename CalculationType = T>
Vector<N, CalculationType> ortho_nn(const std::vector<Vector<N, T>>& points, const std::array<int, N>& indices)
{
        static_assert(N > 1);

        std::array<Vector<N, CalculationType>, N - 1> vectors;

        for (unsigned i = 0; i < N - 1; ++i)
        {
                minus(&vectors[i], points[indices[i + 1]], points[indices[0]]);
        }

        return ortho_nn(vectors);
}

// Единичный вектор e1 из ортогонального дополнения (n-1)-мерного пространства, определяемого n-1 точками и ещё одной точкой.
// Единичный вектор e2 из ортогонального дополнения (n-1)-мерного пространства, определяемого n-1 точками и вектором e1.
template <size_t N, typename T, typename CalculationType>
void ortho_e0_e1(const std::vector<Vector<N, T>>& points, const std::array<int, N - 1>& indices, int point,
                 Vector<N, CalculationType>* e1, Vector<N, CalculationType>* e2)
{
        static_assert(N > 1);

        std::array<Vector<N, CalculationType>, N - 1> vectors;

        for (unsigned i = 0; i < N - 2; ++i)
        {
                minus(&vectors[i], points[indices[i + 1]], points[indices[0]]);
        }

        minus(&vectors[N - 2], points[point], points[indices[0]]);

        *e1 = normalize(ortho_nn(vectors));

        vectors[N - 2] = *e1;

        *e2 = normalize(ortho_nn(vectors));
}
