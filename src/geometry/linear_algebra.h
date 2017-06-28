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

#include "vec.h"
#include "vec_array.h"

#include "com/bits.h"
#include "com/combinatorics.h"
#include "com/error.h"
#include "com/math.h"
#include "com/types.h"

#include <array>
#include <cmath>
#include <gmpxx.h>
#include <numeric>
#include <vector>

template <size_t N>
constexpr std::array<unsigned char, N> sequence_array = make_array_sequence<unsigned char, N>();

//   Расчёт определителей по теореме Лапласа. Элементы одной строки умножаются
// на их алгебраические дополнения.
//   Используется для целочисленных расчётов. Для плавающей точки и больших размерностей
// лучше использовать метод Гаусса.

template <size_t N_V, size_t N_H, typename T, size_t DET_SIZE, typename = void>
struct Determinant
{
        static_assert(N_V >= DET_SIZE);
        static_assert(N_H >= DET_SIZE);
        static_assert(DET_SIZE > 0);

        static T f(const std::array<Vector<N_H, T>, N_V>& vectors, const std::array<unsigned char, DET_SIZE>& v_map,
                   const std::array<unsigned char, DET_SIZE>& h_map)
        {
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

                int coef = 1;
                T det = 0;
                for (unsigned i = 0; i < DET_SIZE; ++i, coef = -coef)
                {
                        T entry = vectors[v_map[0]][h_map[i]];
                        T minor = Determinant<N_V, N_H, T, DET_SIZE - 1>::f(vectors, del_elem(v_map, 0), del_elem(h_map, i));
                        T cofactor = coef * minor;
                        det += entry * cofactor;
                }

                return det;
        }
};

// Определитель матрицы 1x1 равен этому единственному элементу.
template <size_t N_V, size_t N_H, typename T, size_t DET_SIZE>
struct Determinant<N_V, N_H, T, DET_SIZE, std::enable_if_t<DET_SIZE == 1>>
{
        static_assert(N_V >= DET_SIZE);
        static_assert(N_H >= DET_SIZE);

        static T f(const std::array<Vector<N_H, T>, N_V>& vectors, const std::array<unsigned char, DET_SIZE>& v_map,
                   const std::array<unsigned char, DET_SIZE>& h_map)
        {
                return vectors[v_map[0]][h_map[0]];
        }
};

// Просто функция-посредник
template <size_t N_V, size_t N_H, typename T, size_t DET_SIZE>
T determinant(const std::array<Vector<N_H, T>, N_V>& vectors, const std::array<unsigned char, DET_SIZE>& v_map,
              const std::array<unsigned char, DET_SIZE>& h_map)
{
        return Determinant<N_V, N_H, T, DET_SIZE>::f(vectors, v_map, h_map);
}

// Перебор всех вариантов подвекторов размера COUNT, создавая квадратные матрицы размером COUNT на COUNT.
template <int COUNT, size_t N, typename VectorType>
std::enable_if_t<any_integral<VectorType>, bool> linear_independent(const std::array<Vector<N, VectorType>, N>& vectors)
{
        static_assert(N > 1);
        static_assert(COUNT > 0);
        static_assert(COUNT <= N);

        for (const std::array<unsigned char, COUNT>& h_map : get_combinations<N, COUNT>())
        {
                // Используются векторы в количестве COUNT и длиной COUNT
                if (determinant(vectors, sequence_array<COUNT>, h_map) != 0)
                {
                        return true;
                }
        }

        return false;
}

// Вектор из ортогонального дополнения (n-1)-мерного пространства, определяемого n-1 первыми векторами.
template <size_t N, typename T>
Vector<N, T> ortho_nn(const std::array<Vector<N, T>, N - 1>& vectors)
{
        static_assert(N > 1);

        // Для расчёта используются N - 1 строк и N столбцов.
        // Для отображения элементов в элементы используются sequence_array,
        // которые заполнены последовательными числами, начиная с 0.

        Vector<N, T> res;
        T coef = 1;
        for (unsigned i = 0; i < N; ++i, coef = -coef)
        {
                T minor = determinant(vectors, sequence_array<N - 1>, del_elem(sequence_array<N>, i));
                T cofactor = coef * minor;
                res[i] = cofactor;
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

template <typename T>
T cross_2d(const Vector<2, T>& a0, const Vector<2, T>& a1)
{
        return a0[0] * a1[1] - a0[1] * a1[0];
}
