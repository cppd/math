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

/*
 Howard Anton, Chris Rorres.
 Elementary Linear Algebra. 11th Edition.
 Wiley, 2014.

 Раздел 6.3 Gram–Schmidt Process; QR-Decomposition.
*/

#pragma once

#include "linear_algebra.h"

#include "com/vec.h"

#include <array>
#include <utility>

namespace complement_implementation
{
template <size_t ValueIndex, typename T, int... I>
constexpr Vector<sizeof...(I), T> make_vector_one_value(std::integer_sequence<int, I...>, const T& v)
{
        static_assert(ValueIndex >= 0 && ValueIndex < sizeof...(I));

        return Vector<sizeof...(I), T>((I == ValueIndex ? v : 0)...);
}
template <size_t N, typename T, size_t ValueIndex>
constexpr Vector<N, T> make_vector_one_value(const T& v)
{
        return make_vector_one_value<ValueIndex>(std::make_integer_sequence<int, N>(), v);
}
template <typename T, int... I>
constexpr std::array<Vector<sizeof...(I), T>, sizeof...(I)> make_array_of_vectors_one_value(std::integer_sequence<int, I...>,
                                                                                            const T& v)
{
        return {make_vector_one_value<sizeof...(I), T, I>(v)...};
}

template <size_t N, typename T>
inline constexpr std::array<Vector<N, T>, N> orthonormal_set =
        make_array_of_vectors_one_value(std::make_integer_sequence<int, N>(), static_cast<T>(1));

static_assert(orthonormal_set<4, double>[0] == Vector<4, double>(1, 0, 0, 0));
static_assert(orthonormal_set<4, double>[1] == Vector<4, double>(0, 1, 0, 0));
static_assert(orthonormal_set<4, double>[2] == Vector<4, double>(0, 0, 1, 0));
static_assert(orthonormal_set<4, double>[3] == Vector<4, double>(0, 0, 0, 1));

template <typename T>
constexpr T LIMIT = static_cast<T>(0.1);
}

// N - 1 ортогональных единичных вектора, ортогональных заданному единичному вектору

template <size_t N, typename T>
std::array<Vector<N, T>, N - 1> orthogonal_complement_of_unit_vector_by_subspace(const Vector<N, T>& unit_vector)
{
        static_assert(N > 1);

        namespace impl = complement_implementation;

        if constexpr (N == 2)
        {
                return {Vector<2, T>{unit_vector[1], -unit_vector[0]}};
        }

        if constexpr (N == 3)
        {
                Vector<3, T> non_collinear_vector =
                        std::abs(unit_vector[0]) > impl::LIMIT<T> ? Vector<3, T>(0, 1, 0) : Vector<3, T>(1, 0, 0);
                Vector<3, T> e0 = normalize(cross(unit_vector, non_collinear_vector));
                Vector<3, T> e1 = cross(unit_vector, e0);
                return {e0, e1};
        }

        // Найти координатную ось, к которой приближается unit_vector, тогда неколлинеарными
        // векторами к вектору unit_vector будут все остальные координатные оси.
        // Если ни к одной оси не приближается, то использовать любые оси.
        unsigned exclude_axis = 0;
        for (; exclude_axis < N - 2; ++exclude_axis)
        {
                if (std::abs(unit_vector[exclude_axis]) > impl::LIMIT<T>)
                {
                        break;
                }
        }

        // Найти неколлинеарные к исходному вектору unit_vector векторы в количестве N - 2,
        // что вместе с исходным вектором даст N - 1 векторов
        std::array<Vector<N, T>, N - 1> subspace_basis;
        subspace_basis[N - 2] = unit_vector;
        unsigned num = 0;
        for (unsigned i = 0; num < N - 2; ++i)
        {
                if (i != exclude_axis)
                {
                        subspace_basis[num++] = impl::orthonormal_set<N, T>[i];
                }
        }

        // Вычисление векторов из одномерных ортогональных дополнений
        // к исходному вектору и к уже найденным векторам
        for (unsigned i = 0; i < N - 2; ++i)
        {
                subspace_basis[i] = normalize(ortho_nn(subspace_basis));
        }
        subspace_basis[N - 2] = ortho_nn(subspace_basis);

        return subspace_basis;
}

template <size_t N, typename T>
std::array<Vector<N, T>, N - 1> orthogonal_complement_of_unit_vector_by_gram_schmidt(const Vector<N, T>& unit_vector)
{
        static_assert(N > 1);

        namespace impl = complement_implementation;

        if constexpr (N == 2)
        {
                return {Vector<2, T>(unit_vector[1], -unit_vector[0])};
        }

        // Найти координатную ось, к которой приближается unit_vector, тогда неколлинеарными
        // векторами к вектору unit_vector будут все остальные координатные оси.
        // Если ни к одной оси не приближается, то использовать любые оси.
        unsigned exclude_axis = 0;
        for (; exclude_axis < N - 1; ++exclude_axis)
        {
                if (std::abs(unit_vector[exclude_axis]) > impl::LIMIT<T>)
                {
                        break;
                }
        }

        // Найти базис из вектора unit_vector и неколлинеарных ему векторов ортонормированного базиса
        std::array<Vector<N, T>, N> basis;
        basis[0] = unit_vector;
        unsigned num = 1;
        for (unsigned i = 0; num < N; ++i)
        {
                if (i != exclude_axis)
                {
                        basis[num++] = impl::orthonormal_set<N, T>[i];
                }
        }

        // Процесс Грама-Шмидта.
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
                orthogonal_basis[i] = normalize(basis[i] - sum);
        }

        // Выбросить исходный вектор, который находится по индексу 0
        std::array<Vector<N, T>, N - 1> res;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                res[i] = orthogonal_basis[i + 1];
        }

        return res;
}

template <size_t N, typename T>
std::array<Vector<N, T>, N - 1> orthogonal_complement_of_unit_vector(const Vector<N, T>& unit_vector)
{
        if constexpr (N <= 4)
        {
                return orthogonal_complement_of_unit_vector_by_subspace(unit_vector);
        }
        if constexpr (N >= 5)
        {
                return orthogonal_complement_of_unit_vector_by_gram_schmidt(unit_vector);
        }
}
