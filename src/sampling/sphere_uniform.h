/*
Copyright (C) 2017-2023 Topological Manifold

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

13.6 2D Sampling with multidimensional transformations
*/

/*
Donald Knuth.
The Art of Computer Programming. Second edition.
Addison-Wesley, 1981.

Volume 2. Seminumerical Algorithms
3.4.1. Numerical Distributions
E. Other continuous distributions
(6) Random point on n-dimensional sphere with radius one
*/

#pragma once

#include <src/com/constant.h>
#include <src/com/exponent.h>
#include <src/geometry/shapes/ball_volume.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <random>
#include <utility>

namespace ns::sampling
{
namespace sphere_implementation
{
template <std::size_t N, typename T, typename RandomEngine, typename Distribution>
Vector<N, T> random_vector(RandomEngine& engine, Distribution& distribution)
{
        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                return Vector<N, T>((static_cast<void>(I), distribution(engine))...);
        }(std::make_integer_sequence<std::size_t, N>());
}

//

template <typename RandomEngine, std::size_t N, typename T>
void uniform_in_sphere_by_rejection(RandomEngine& engine, Vector<N, T>& v, T& v_length_square)
{
        static_assert(N >= 1);

        thread_local std::uniform_real_distribution<T> urd(-1, 1);

        while (true)
        {
                v = random_vector<N, T>(engine, urd);
                v_length_square = dot(v, v);
                if (v_length_square <= 1 && v_length_square > 0)
                {
                        break;
                }
        }
}

template <typename RandomEngine, std::size_t N, typename T>
void uniform_in_sphere_by_normal_distribution(RandomEngine& engine, Vector<N, T>& v, T& v_length_square)
{
        static_assert(N >= 1);

        thread_local std::normal_distribution<T> nd(0, 1);

        v = random_vector<N, T>(engine, nd).normalized();

        thread_local std::uniform_real_distribution<T> urd(0, 1);

        T k = urd(engine);
        if constexpr (N == 2)
        {
                k = std::sqrt(k);
        }
        else if constexpr (N == 4)
        {
                k = std::sqrt(std::sqrt(k));
        }
        else
        {
                k = std::pow(k, T{1} / N);
        }
        v *= k;
        v_length_square = k * k;
}

//

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> uniform_on_sphere_by_rejection(RandomEngine& engine)
{
        static_assert(N >= 2);

        thread_local std::uniform_real_distribution<T> urd(-1, 1);

        Vector<N, T> v;
        while (true)
        {
                v = random_vector<N, T>(engine, urd);
                const T length_square = dot(v, v);
                if (length_square <= 1)
                {
                        const T length = std::sqrt(length_square);
                        if (length > 0)
                        {
                                v /= length;
                                return v;
                        }
                }
        }
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> uniform_on_sphere_by_normal_distribution(RandomEngine& engine)
{
        static_assert(N >= 2);

        thread_local std::normal_distribution<T> nd(0, 1);

        return random_vector<N, T>(engine, nd).normalized();
}
}

template <typename RandomEngine, std::size_t N, typename T>
void uniform_in_sphere(RandomEngine& engine, Vector<N, T>& v, T& v_length_square)
{
        namespace impl = sphere_implementation;

        if constexpr (N <= 4)
        {
                impl::uniform_in_sphere_by_rejection(engine, v, v_length_square);
        }
        else
        {
                impl::uniform_in_sphere_by_normal_distribution(engine, v, v_length_square);
        }
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> uniform_on_sphere(RandomEngine& engine)
{
        namespace impl = sphere_implementation;

        if constexpr (N <= 4)
        {
                return impl::uniform_on_sphere_by_rejection<N, T>(engine);
        }
        else
        {
                return impl::uniform_on_sphere_by_normal_distribution<N, T>(engine);
        }
}

template <std::size_t N, typename T, std::size_t M, typename RandomEngine>
Vector<N, T> uniform_in_sphere(RandomEngine& engine, const std::array<Vector<N, T>, M>& orthogonal_vectors)
{
        static_assert(N > 0 && M > 0 && M <= N);

        Vector<M, T> v;
        T v_length_square;
        uniform_in_sphere(engine, v, v_length_square);

        Vector<N, T> res = orthogonal_vectors[0] * v[0];
        for (std::size_t i = 1; i < M; ++i)
        {
                res.multiply_add(orthogonal_vectors[i], v[i]);
        }
        return res;
}

template <std::size_t N, typename T>
constexpr T uniform_in_sphere_pdf(const T& radius)
{
        constexpr T PDF = 1 / geometry::shapes::BALL_VOLUME<N, long double>;
        return PDF / power<N>(radius);
}

template <std::size_t N, typename T>
constexpr T uniform_on_sphere_pdf()
{
        constexpr T PDF = 1 / geometry::shapes::SPHERE_AREA<N, long double>;
        return PDF;
}

template <std::size_t N, typename T>
constexpr T uniform_on_hemisphere_pdf()
{
        constexpr T PDF = 2 / geometry::shapes::SPHERE_AREA<N, long double>;
        return PDF;
}
}
