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

#include "determinant.h"
#include "difference.h"
#include "vec.h"

#include <src/com/arrays.h>

#include <array>
#include <vector>

namespace ns
{
// Вектор из ортогонального дополнения (n-1)-мерного подпространства
template <size_t N, typename T>
Vector<N, T> ortho_nn(const std::array<Vector<N, T>, N - 1>& vectors)
{
        static_assert(N > 1);

        // Для расчёта используются N - 1 строк и N столбцов.
        // Для отображения элементов в элементы используются sequence_array,
        // которые заполнены последовательными числами, начиная с 0.

        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                T minor = determinant(vectors, sequence_uchar_array<N - 1>, del_elem(sequence_uchar_array<N>, i));
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

// Вектор из ортогонального дополнения (n-1)-мерного пространства, определяемого n точками
template <size_t N, typename T, typename CalculationType = T>
Vector<N, CalculationType> ortho_nn(const std::vector<Vector<N, T>>& points, const std::array<int, N>& indices)
{
        static_assert(N > 1);

        std::array<Vector<N, CalculationType>, N - 1> vectors;

        for (unsigned i = 0; i < N - 1; ++i)
        {
                difference(&vectors[i], points[indices[i + 1]], points[indices[0]]);
        }

        return ortho_nn(vectors);
}

// Единичный вектор e1 из ортогонального дополнения (n-1)-мерного пространства, определяемого n-1 точками и ещё одной точкой.
// Единичный вектор e2 из ортогонального дополнения (n-1)-мерного пространства, определяемого n-1 точками и вектором e1.
template <size_t N, typename T, typename CalculationType>
void ortho_e0_e1(
        const std::vector<Vector<N, T>>& points,
        const std::array<int, N - 1>& indices,
        int point,
        Vector<N, CalculationType>* e1,
        Vector<N, CalculationType>* e2)
{
        static_assert(N > 1);

        std::array<Vector<N, CalculationType>, N - 1> vectors;

        for (unsigned i = 0; i < N - 2; ++i)
        {
                difference(&vectors[i], points[indices[i + 1]], points[indices[0]]);
        }

        difference(&vectors[N - 2], points[point], points[indices[0]]);

        *e1 = ortho_nn(vectors).normalized();

        vectors[N - 2] = *e1;

        *e2 = ortho_nn(vectors).normalized();
}
}
