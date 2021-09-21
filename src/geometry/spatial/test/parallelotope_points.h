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
template <typename T>
inline constexpr T POSITION_DELTA;
template <>
inline constexpr double POSITION_DELTA<double> = 1e-6;

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<Vector<Parallelotope::SPACE_DIMENSION, typename Parallelotope::DataType>> external_points(
        RandomEngine& engine,
        const int count,
        const Parallelotope& p,
        std::integer_sequence<std::size_t, I...>&&)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        const std::array<T, N> len = {p.e(I).norm()...};

        std::array<std::uniform_real_distribution<T>, N> low_urd = {
                std::uniform_real_distribution<T>{-len[I] * 10, -POSITION_DELTA<T> * len[I]}...};

        std::array<std::uniform_real_distribution<T>, N> high_urd = {
                std::uniform_real_distribution<T>{len[I] * (1 + POSITION_DELTA<T>), len[I] * 10}...};

        const std::array<Vector<N, T>, N> unit = {(p.e(I) / len[I])...};

        std::bernoulli_distribution rnd(0.5);
        std::vector<Vector<N, T>> points;
        points.reserve(count);

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v((rnd(engine) ? low_urd[I](engine) : high_urd[I](engine))...);

                points.push_back(p.org() + ((unit[I] * v[I]) + ...));
        }

        return points;
}

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<Vector<Parallelotope::SPACE_DIMENSION, typename Parallelotope::DataType>> internal_points(
        RandomEngine& engine,
        const int count,
        const Parallelotope& p,
        std::integer_sequence<std::size_t, I...>&&)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        const std::array<T, N> len = {p.e(I).norm()...};

        std::array<std::uniform_real_distribution<T>, N> urd = {
                std::uniform_real_distribution<T>{len[I] * POSITION_DELTA<T>, len[I] * (1 - POSITION_DELTA<T>)}...};

        const std::array<Vector<N, T>, N> unit = {(p.e(I) / len[I])...};

        std::vector<Vector<N, T>> points;
        points.reserve(count);

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v((urd[I](engine))...);

                points.push_back(p.org() + ((unit[I] * v[I]) + ...));
        }

        return points;
}

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<Vector<Parallelotope::SPACE_DIMENSION, typename Parallelotope::DataType>> cover_points(
        RandomEngine& engine,
        const int count,
        const Parallelotope& p,
        std::integer_sequence<std::size_t, I...>&&)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        const std::array<T, N> len = {p.e(I).norm()...};

        std::array<std::uniform_real_distribution<T>, N> cover_urd = {
                std::uniform_real_distribution<T>{static_cast<T>(-0.2) * len[I], len[I] * static_cast<T>(1.2)}...};

        std::array<std::uniform_real_distribution<T>, N> len_urd = {std::uniform_real_distribution<T>{0, len[I]}...};

        const std::array<Vector<N, T>, N> unit = {(p.e(I) / len[I])...};

        std::vector<Vector<N, T>> points;
        points.reserve(count * (1 + N * 2));

        for (int i = 0; i < count; ++i)
        {
                points.push_back(p.org() + ((unit[I] * cover_urd[I](engine)) + ...));

                for (unsigned n = 0; n < N; ++n)
                {
                        Vector<N, T> v;

                        v = p.org();
                        for (unsigned d = 0; d < N; ++d)
                        {
                                if (d != n)
                                {
                                        v += unit[d] * len_urd[d](engine);
                                }
                        }
                        points.push_back(v);

                        v = p.org();
                        for (unsigned d = 0; d < N; ++d)
                        {
                                if (d != n)
                                {
                                        v += unit[d] * len_urd[d](engine);
                                }
                        }
                        points.push_back(v + p.e(n));
                }
        }

        return points;
}
}

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<Vector<Parallelotope::SPACE_DIMENSION, typename Parallelotope::DataType>> external_points(
        RandomEngine& engine,
        const int count,
        const Parallelotope& p)
{
        namespace impl = parallelotope_points_implementation;
        return impl::external_points(
                engine, count, p, std::make_integer_sequence<std::size_t, Parallelotope::SPACE_DIMENSION>());
}

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<Vector<Parallelotope::SPACE_DIMENSION, typename Parallelotope::DataType>> internal_points(
        RandomEngine& engine,
        const int count,
        const Parallelotope& p)
{
        namespace impl = parallelotope_points_implementation;
        return impl::internal_points(
                engine, count, p, std::make_integer_sequence<std::size_t, Parallelotope::SPACE_DIMENSION>());
}

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<Vector<Parallelotope::SPACE_DIMENSION, typename Parallelotope::DataType>> cover_points(
        RandomEngine& engine,
        const int count,
        const Parallelotope& p)
{
        namespace impl = parallelotope_points_implementation;
        return impl::cover_points(
                engine, count, p, std::make_integer_sequence<std::size_t, Parallelotope::SPACE_DIMENSION>());
}
}
