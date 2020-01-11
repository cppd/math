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
 Matt Pharr, Wenzel Jakob, Greg Humphreys.
 Physically Based Rendering. From theory to implementation. Third edition.
 Elsevier, 2017.
 13.6 2D Sampling with multidimensional transformations.

 Donald Knuth.
 The Art of Computer Programming. Second edition.
 Addison-Wesley, 1981.
 Volume 2. Seminumerical Algorithms.
 3.4.1. Numerical Distributions.
 E. Other continuous distributions.
 (6) Random point on n-dimensional sphere with radius one.
*/

#pragma once

#include "com/random/vector.h"
#include "com/vec.h"
#include "geometry/core/complement.h"

#include <cmath>
#include <random>

template <typename RandomEngine, size_t N, typename T>
void random_in_sphere_by_rejection(RandomEngine& random_engine, Vector<N, T>& v, T& v_length_square)
{
        static_assert(N >= 2);

        std::uniform_real_distribution<T> urd(-1, 1);
        while (true)
        {
                v = random_vector<N, T>(random_engine, urd);
                v_length_square = dot(v, v);
                if (v_length_square <= 1 && v_length_square > 0)
                {
                        break;
                }
        }
}

template <typename RandomEngine, size_t N, typename T>
void random_in_sphere_by_normal_distribution(RandomEngine& random_engine, Vector<N, T>& v, T& v_length_square)
{
        static_assert(N >= 2);

        std::normal_distribution<T> nd(0, 1);
        v = random_vector<N, T>(random_engine, nd).normalized();

        std::uniform_real_distribution<T> urd(0, 1);
        T k = std::pow(urd(random_engine), 1.0 / N);
        v *= k;
        v_length_square = k * k;
}

template <typename RandomEngine, size_t N, typename T>
void random_in_sphere(RandomEngine& random_engine, Vector<N, T>& v, T& v_length_square)
{
        if constexpr (N <= 5)
        {
                random_in_sphere_by_rejection(random_engine, v, v_length_square);
        }
        if constexpr (N >= 6)
        {
                random_in_sphere_by_normal_distribution(random_engine, v, v_length_square);
        }
}

template <typename RandomEngine, size_t N, typename T>
Vector<N, T> random_cosine_weighted_on_hemisphere(RandomEngine& random_engine, const Vector<N, T>& normal)
{
        static_assert(N > 2);

        Vector<N - 1, T> v;
        T v_length_square;

        random_in_sphere(random_engine, v, v_length_square);

        if constexpr (N >= 4)
        {
                T k = std::pow(v_length_square, 0.5 * (0.5 * N - 1.5));
                v *= k;
                v_length_square *= k * k;
        }

        T n = std::sqrt(1 - v_length_square);

        std::array<Vector<N, T>, N - 1> basis = orthogonal_complement_of_unit_vector(normal);

        Vector<N, T> res = n * normal;

        for (unsigned i = 0; i < N - 1; ++i)
        {
                res += v[i] * basis[i];
        }

        return res;
}

#if 0
template <size_t N, typename T, typename RandomEngine>
Vector<N, T> random_in_sphere(RandomEngine& random_engine)
{
        std::uniform_real_distribution<T> urd(-1, 1);

        while (true)
        {
                Vector<N, T> v = random_vector<N, T>(random_engine, urd);
                T length_square = dot(v, v);
                if (length_square <= 1 && length_square > 0)
                {
                        return v;
                }
        }
}

template <size_t N, typename T, typename RandomEngine>
Vector<N, T> random_in_hemisphere(RandomEngine& random_engine, const Vector<N, T>& normal)
{
        Vector<N, T> v = random_in_sphere<N, T>(random_engine);

        return (dot(v, normal) >= 0) ? v : -v;
}
#endif

// Вариант алгоритма для равномерных точек на диске.
// Работает медленнее алгоритма с выбрасыванием значений.
//   std::uniform_real_distribution<T> urd(0, 1);
//   v_length_square = urd(random_engine);
//   T theta = 2 * static_cast<T>(PI) * urd(random_engine);
//   T r = std::sqrt(v_length_square);
//   x = r * std::cos(theta);
//   y = r * std::sin(theta);
