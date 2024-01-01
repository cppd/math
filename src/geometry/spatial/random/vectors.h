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

#include <src/com/error.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <type_traits>

namespace ns::geometry::spatial::random
{
namespace vectors_implementation
{
template <std::size_t M, std::size_t N, typename T>
bool check_vectors(const T& min_length, const T& max_length, const std::array<Vector<N, T>, M>& vectors)
{
        constexpr T MAX_DOT_PRODUCT = 0.9;

        std::array<Vector<N, T>, M> unit_vectors = vectors;
        for (Vector<N, T>& v : unit_vectors)
        {
                const T length = v.norm();
                if (!(length >= min_length && length <= max_length))
                {
                        return false;
                }
                v /= length;
        }

        for (std::size_t i = 0; i < M; ++i)
        {
                for (std::size_t j = i + 1; j < M; ++j)
                {
                        if (!(std::abs(dot(unit_vectors[i], unit_vectors[j])) < MAX_DOT_PRODUCT))
                        {
                                return false;
                        }
                }
        }

        return true;
}
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> point(const std::type_identity_t<T>& interval, RandomEngine& engine)
{
        ASSERT(interval > 0);

        std::uniform_real_distribution<T> urd(-interval, interval);

        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = urd(engine);
        }
        return res;
}

template <std::size_t M, std::size_t N, typename T, typename RandomEngine>
std::array<Vector<N, T>, M> vectors(
        const std::type_identity_t<T>& min_length,
        const std::type_identity_t<T>& max_length,
        RandomEngine& engine)
{
        static_assert(M > 0 && M <= N);

        ASSERT(min_length > 0 && min_length < max_length);

        std::uniform_real_distribution<T> urd(min_length, max_length);
        std::array<Vector<N, T>, M> vectors;

        while (true)
        {
                for (Vector<N, T>& v : vectors)
                {
                        v = urd(engine) * sampling::uniform_on_sphere<N, T>(engine);
                }

                if (vectors_implementation::check_vectors(min_length, max_length, vectors))
                {
                        return vectors;
                }
        }
}

template <std::size_t N, typename T, typename RandomEngine>
std::array<T, N> aa_vectors(
        const std::type_identity_t<T>& min_length,
        const std::type_identity_t<T>& max_length,
        RandomEngine& engine)
{
        ASSERT(min_length > 0 && min_length < max_length);

        std::uniform_real_distribution<T> urd(min_length, max_length);

        std::array<T, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = urd(engine);
        }
        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> direction_for_normal(const T& from, const T& to, const Vector<N, T>& normal, RandomEngine& engine)
{
        ASSERT(from >= 0 && from < to);

        while (true)
        {
                const Vector<N, T> direction = sampling::uniform_on_sphere<N, T>(engine);
                const T d = dot(normal, direction);

                if (!(std::abs(d) >= from && std::abs(d) <= to))
                {
                        continue;
                }

                return (d < 0) ? -direction : direction;
        }
}
}
