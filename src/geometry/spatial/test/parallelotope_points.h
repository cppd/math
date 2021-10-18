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

#pragma once

#include <src/numerical/vec.h>

#include <array>
#include <random>
#include <utility>
#include <vector>

namespace ns::geometry::spatial::test
{
namespace parallelotope_points_implementation
{
template <std::size_t N, typename T, typename RandomEngine, std::size_t... I>
std::vector<Vector<N, T>> external_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine,
        std::integer_sequence<std::size_t, I...>&&)
{
        static_assert(sizeof...(I) == N);

        const std::array<T, N> len = {vectors[I].norm()...};

        std::array<std::uniform_real_distribution<T>, N> low_urd = {
                std::uniform_real_distribution<T>{T(-10) * len[I], T(-0.01) * len[I]}...};

        std::array<std::uniform_real_distribution<T>, N> high_urd = {
                std::uniform_real_distribution<T>{T(1.01) * len[I], T(10) * len[I]}...};

        const std::array<Vector<N, T>, N> unit = {(vectors[I] / len[I])...};

        std::bernoulli_distribution rnd(0.5);
        std::vector<Vector<N, T>> points;
        points.reserve(count);

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v((rnd(engine) ? low_urd[I](engine) : high_urd[I](engine))...);

                points.push_back(org + ((unit[I] * v[I]) + ...));
        }

        return points;
}

template <std::size_t N, typename T, typename RandomEngine, std::size_t... I>
std::vector<Vector<N, T>> internal_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine,
        std::integer_sequence<std::size_t, I...>&&)
{
        static_assert(sizeof...(I) == N);

        const std::array<T, N> len = {vectors[I].norm()...};

        std::array<std::uniform_real_distribution<T>, N> urd = {
                std::uniform_real_distribution<T>{T(0.01) * len[I], T(0.99) * len[I]}...};

        const std::array<Vector<N, T>, N> unit = {(vectors[I] / len[I])...};

        std::vector<Vector<N, T>> points;
        points.reserve(count);

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v((urd[I](engine))...);

                points.push_back(org + ((unit[I] * v[I]) + ...));
        }

        return points;
}

template <std::size_t N, typename T, typename RandomEngine, std::size_t... I>
std::vector<Vector<N, T>> cover_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine,
        std::integer_sequence<std::size_t, I...>&&)
{
        static_assert(sizeof...(I) == N);

        const std::array<T, N> len = {vectors[I].norm()...};

        std::array<std::uniform_real_distribution<T>, N> cover_urd = {
                std::uniform_real_distribution<T>{T(-0.2) * len[I], T(1.2) * len[I]}...};

        std::array<std::uniform_real_distribution<T>, N> len_urd = {std::uniform_real_distribution<T>{0, len[I]}...};

        const std::array<Vector<N, T>, N> unit = {(vectors[I] / len[I])...};

        std::vector<Vector<N, T>> points;
        points.reserve(count * (1 + N * 2));

        for (int i = 0; i < count; ++i)
        {
                points.push_back(org + ((unit[I] * cover_urd[I](engine)) + ...));

                for (unsigned n = 0; n < N; ++n)
                {
                        Vector<N, T> v;

                        v = org;
                        for (unsigned d = 0; d < N; ++d)
                        {
                                if (d != n)
                                {
                                        v += unit[d] * len_urd[d](engine);
                                }
                        }
                        points.push_back(v);

                        v = org;
                        for (unsigned d = 0; d < N; ++d)
                        {
                                if (d != n)
                                {
                                        v += unit[d] * len_urd[d](engine);
                                }
                        }
                        points.push_back(v + vectors[n]);
                }
        }

        return points;
}
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> external_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine)
{
        namespace impl = parallelotope_points_implementation;
        return impl::external_points(org, vectors, count, engine, std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> internal_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine)
{
        namespace impl = parallelotope_points_implementation;
        return impl::internal_points(org, vectors, count, engine, std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> cover_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine)
{
        namespace impl = parallelotope_points_implementation;
        return impl::cover_points(org, vectors, count, engine, std::make_integer_sequence<std::size_t, N>());
}
}
