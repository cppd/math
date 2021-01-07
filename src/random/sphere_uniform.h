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

#include <src/numerical/vec.h>

#include <cmath>
#include <random>
#include <utility>

namespace ns::random
{
namespace sphere_implementation
{
template <typename T, typename RandomEngine, typename Distribution, std::size_t... I>
Vector<sizeof...(I), T> random_vector(
        RandomEngine& engine,
        Distribution& distribution,
        std::integer_sequence<std::size_t, I...>)
{
        return Vector<sizeof...(I), T>((static_cast<void>(I), distribution(engine))...);
}

template <std::size_t N, typename T, typename RandomEngine, typename Distribution>
Vector<N, T> random_vector(RandomEngine& engine, Distribution& distribution)
{
        return random_vector<T>(engine, distribution, std::make_integer_sequence<std::size_t, N>());
}

//

template <typename RandomEngine, std::size_t N, typename T>
void random_in_sphere_by_rejection(RandomEngine& random_engine, Vector<N, T>& v, T& v_length_square)
{
        static_assert(N >= 2);

        thread_local std::uniform_real_distribution<T> urd(-1, 1);

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

template <typename RandomEngine, std::size_t N, typename T>
void random_in_sphere_by_normal_distribution(RandomEngine& random_engine, Vector<N, T>& v, T& v_length_square)
{
        static_assert(N >= 2);

        thread_local std::normal_distribution<T> nd(0, 1);

        v = random_vector<N, T>(random_engine, nd).normalized();

        thread_local std::uniform_real_distribution<T> urd(0, 1);

        T k = std::pow(urd(random_engine), T(1) / N);
        v *= k;
        v_length_square = k * k;
}

//

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_on_sphere_by_rejection(RandomEngine& random_engine)
{
        static_assert(N >= 2);

        thread_local std::uniform_real_distribution<T> urd(-1, 1);

        Vector<N, T> v;
        while (true)
        {
                v = random_vector<N, T>(random_engine, urd);
                T length_square = dot(v, v);
                if (length_square <= 1)
                {
                        T length = std::sqrt(length_square);
                        if (length > 0)
                        {
                                v /= length;
                                return v;
                        }
                }
        }
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_on_sphere_by_normal_distribution(RandomEngine& random_engine)
{
        static_assert(N >= 2);

        thread_local std::normal_distribution<T> nd(0, 1);

        return random_vector<N, T>(random_engine, nd).normalized();
}
}

template <typename RandomEngine, std::size_t N, typename T>
void random_in_sphere(RandomEngine& random_engine, Vector<N, T>& v, T& v_length_square)
{
        namespace impl = sphere_implementation;

        if constexpr (N <= 5)
        {
                impl::random_in_sphere_by_rejection(random_engine, v, v_length_square);
        }
        else
        {
                impl::random_in_sphere_by_normal_distribution(random_engine, v, v_length_square);
        }
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_on_sphere(RandomEngine& random_engine)
{
        namespace impl = sphere_implementation;

        if constexpr (N <= 4)
        {
                return impl::random_on_sphere_by_rejection<N, T>(random_engine);
        }
        else
        {
                return impl::random_on_sphere_by_normal_distribution<N, T>(random_engine);
        }
}

// Вариант алгоритма для равномерных точек на диске.
// Работает медленнее алгоритма с выбрасыванием значений.
//   std::uniform_real_distribution<T> urd(0, 1);
//   v_length_square = urd(random_engine);
//   T theta = 2 * static_cast<T>(PI) * urd(random_engine);
//   T r = std::sqrt(v_length_square);
//   x = r * std::cos(theta);
//   y = r * std::sin(theta);
}
