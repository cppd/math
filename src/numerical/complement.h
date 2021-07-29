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

/*
Howard Anton, Chris Rorres.
Elementary Linear Algebra. 11th Edition.
Wiley, 2014.

6.3 Gram–Schmidt Process; QR-Decomposition
*/

#pragma once

#include "determinant.h"
#include "difference.h"
#include "identity.h"
#include "vec.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/mpz.h>

#include <array>
#include <cmath>
#include <vector>

namespace ns::numerical
{
template <std::size_t N, typename T>
Vector<N, T> orthogonal_complement(const std::array<Vector<N, T>, N - 1>& vectors)
{
        static_assert(N >= 5);

        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                T minor = determinant_by_cofactor_expansion(
                        vectors, sequence_uchar_array<N - 1>, del_elem(sequence_uchar_array<N>, i));
                res[i] = (i & 1) ? -minor : minor;
        }
        return res;
}

template <typename T>
Vector<2, T> orthogonal_complement(const std::array<Vector<2, T>, 1>& v)
{
        return Vector<2, T>(v[0][1], -v[0][0]);
}

template <typename T>
Vector<3, T> orthogonal_complement(const std::array<Vector<3, T>, 2>& v)
{
        Vector<3, T> res;
        res[0] = +(v[0][1] * v[1][2] - v[0][2] * v[1][1]);
        res[1] = -(v[0][0] * v[1][2] - v[0][2] * v[1][0]);
        res[2] = +(v[0][0] * v[1][1] - v[0][1] * v[1][0]);
        return res;
}

template <typename T>
Vector<4, T> orthogonal_complement(const std::array<Vector<4, T>, 3>& v)
{
        static_assert(!std::is_same_v<mpz_class, T>);

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

inline Vector<4, mpz_class> orthogonal_complement(const std::array<Vector<4, mpz_class>, 3>& v)
{
        const auto add_mul = [](mpz_class* const res, const mpz_class& a, const mpz_class& b, const mpz_class& c,
                                const mpz_class& d, const mpz_class& e)
        {
                mpz_mul(res->get_mpz_t(), b.get_mpz_t(), c.get_mpz_t());
                mpz_submul(res->get_mpz_t(), d.get_mpz_t(), e.get_mpz_t());
                mpz_mul(res->get_mpz_t(), a.get_mpz_t(), res->get_mpz_t());
        };

        const auto res_1 = [](mpz_class* const res, const mpz_class& a, const mpz_class& b, const mpz_class& c)
        {
                *res = a;
                mpz_sub(res->get_mpz_t(), res->get_mpz_t(), b.get_mpz_t());
                mpz_add(res->get_mpz_t(), res->get_mpz_t(), c.get_mpz_t());
        };

        const auto res_2 = [](mpz_class* const res, const mpz_class& a, const mpz_class& b, const mpz_class& c)
        {
                *res = b;
                mpz_sub(res->get_mpz_t(), res->get_mpz_t(), a.get_mpz_t());
                mpz_sub(res->get_mpz_t(), res->get_mpz_t(), c.get_mpz_t());
        };

        thread_local Vector<4, mpz_class> res;

        thread_local mpz_class x1;
        thread_local mpz_class x2;
        thread_local mpz_class x3;

        add_mul(&x1, v[0][1], v[1][2], v[2][3], v[1][3], v[2][2]);
        add_mul(&x2, v[0][2], v[1][1], v[2][3], v[1][3], v[2][1]);
        add_mul(&x3, v[0][3], v[1][1], v[2][2], v[1][2], v[2][1]);
        res_1(&res[0], x1, x2, x3);

        add_mul(&x1, v[0][0], v[1][2], v[2][3], v[1][3], v[2][2]);
        add_mul(&x2, v[0][2], v[1][0], v[2][3], v[1][3], v[2][0]);
        add_mul(&x3, v[0][3], v[1][0], v[2][2], v[1][2], v[2][0]);
        res_2(&res[1], x1, x2, x3);

        add_mul(&x1, v[0][0], v[1][1], v[2][3], v[1][3], v[2][1]);
        add_mul(&x2, v[0][1], v[1][0], v[2][3], v[1][3], v[2][0]);
        add_mul(&x3, v[0][3], v[1][0], v[2][1], v[1][1], v[2][0]);
        res_1(&res[2], x1, x2, x3);

        add_mul(&x1, v[0][0], v[1][1], v[2][2], v[1][2], v[2][1]);
        add_mul(&x2, v[0][1], v[1][0], v[2][2], v[1][2], v[2][0]);
        add_mul(&x3, v[0][2], v[1][0], v[2][1], v[1][1], v[2][0]);
        res_2(&res[3], x1, x2, x3);

        return res;
}

template <std::size_t N, typename T, typename CalculationType = T>
Vector<N, CalculationType> orthogonal_complement(
        const std::vector<Vector<N, T>>& points,
        const std::array<int, N>& indices)
{
        static_assert(N > 1);

        std::array<Vector<N, CalculationType>, N - 1> vectors;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                difference(&vectors[i], points[indices[i + 1]], points[indices[0]]);
        }
        return orthogonal_complement(vectors);
}

//

namespace complement_implementation
{
template <std::size_t N, typename T>
std::array<Vector<N, T>, N - 1> orthogonal_complement_by_subspace(const Vector<N, T>& unit_vector)
{
        static_assert(N > 1);

        static constexpr T LIMIT = 0.5;

        if constexpr (N == 2)
        {
                return {Vector<2, T>{unit_vector[1], -unit_vector[0]}};
        }

        if constexpr (N == 3)
        {
                static constexpr Vector<3, T> X(1, 0, 0);
                static constexpr Vector<3, T> Y(0, 1, 0);
                const Vector<3, T>& v = std::abs(unit_vector[0]) > LIMIT ? Y : X;
                Vector<3, T> e0 = cross(unit_vector, v).normalized();
                Vector<3, T> e1 = cross(unit_vector, e0);
                return {e0, e1};
        }

        // Найти координатную ось, к которой приближается unit_vector,
        // тогда неколлинеарными векторами к unit_vector будут все
        // остальные координатные оси.
        const unsigned exclude_axis = [&]
        {
                unsigned i = 0;
                for (; i < N - 2; ++i)
                {
                        if (std::abs(unit_vector[i]) > LIMIT)
                        {
                                break;
                        }
                }
                return i;
        }();

        // Найти неколлинеарные к unit_vector векторы в количестве
        // N - 2, что вместе с unit_vector даст N - 1 векторов
        std::array<Vector<N, T>, N - 1> subspace_basis;
        for (unsigned i = 0, num = 0; num < N - 2; ++i)
        {
                if (i != exclude_axis)
                {
                        subspace_basis[num++] = identity_array<N, T>[i];
                }
        }
        subspace_basis[N - 2] = unit_vector;

        // Вычисление векторов из одномерных ортогональных дополнений
        // к исходному вектору и к уже найденным векторам
        for (unsigned i = 0; i < N - 2; ++i)
        {
                subspace_basis[i] = orthogonal_complement(subspace_basis).normalized();
        }
        subspace_basis[N - 2] = orthogonal_complement(subspace_basis);

        return subspace_basis;
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N - 1> orthogonal_complement_by_gram_schmidt(const Vector<N, T>& unit_vector)
{
        static_assert(N > 1);

        // Найти самую близкую к unit_vector координатную ось, тогда
        // unit_vector и остальные оси не будут линейно зависимыми.
        // Или можно найти первую ось, где abs(unit_vector[i])
        // больше (constant < 1) * sqrt(1/(N-1)).
        const unsigned exclude_axis = [&]
        {
                unsigned i_max = 0;
                T v_max = std::abs(unit_vector[0]);
                for (unsigned i = 1; i < N; ++i)
                {
                        T v = std::abs(unit_vector[i]);
                        if (v > v_max)
                        {
                                i_max = i;
                                v_max = v;
                        }
                }
                return i_max;
        }();

        std::array<Vector<N, T>, N> basis;
        basis[0] = unit_vector;
        for (unsigned i = 0, num = 1; i < N; ++i)
        {
                if (i != exclude_axis)
                {
                        basis[num++] = identity_array<N, T>[i];
                }
        }

        // Из неортогонального базиса делается ортогональный базис.
        std::array<Vector<N, T>, N> orthogonal_basis = basis;
        for (unsigned i = 1; i < N; ++i)
        {
                // Делить на длину не надо, так как найденные векторы ортогонального
                // базиса тут же приводятся к единичной длине
                Vector<N, T> sum(0);
                for (unsigned n = 0; n < i; ++n)
                {
                        sum += dot(basis[i], orthogonal_basis[n]) * orthogonal_basis[n];
                }
                orthogonal_basis[i] = (basis[i] - sum).normalized();
        }

        // Выбросить исходный вектор, который находится по индексу 0
        std::array<Vector<N, T>, N - 1> res;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                res[i] = orthogonal_basis[i + 1];
        }

        return res;
}
}

// orthonormal orthogonal complement
template <std::size_t N, typename T>
std::array<Vector<N, T>, N - 1> orthogonal_complement_of_unit_vector(const Vector<N, T>& unit_vector)
{
        ASSERT(unit_vector.is_unit());

        namespace impl = complement_implementation;

        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
        {
                if constexpr (N <= 4)
                {
                        return impl::orthogonal_complement_by_subspace(unit_vector);
                }
                else
                {
                        return impl::orthogonal_complement_by_gram_schmidt(unit_vector);
                }
        }
        if constexpr (std::is_same_v<T, long double>)
        {
                if constexpr (N <= 6)
                {
                        return impl::orthogonal_complement_by_subspace(unit_vector);
                }
                else
                {
                        return impl::orthogonal_complement_by_gram_schmidt(unit_vector);
                }
        }
}
}
