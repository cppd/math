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

#pragma once

#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <random>
#include <vector>

namespace ns::geometry::spatial::random
{
template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> parallelotope_external_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine)
{
        std::uniform_real_distribution<T> low_urd(-10, -0.01);
        std::uniform_real_distribution<T> high_urd(1.01, 10);
        std::bernoulli_distribution bd(0.5);

        const auto random_point = [&]()
        {
                Vector<N, T> res = org;
                for (std::size_t i = 0; i < N; ++i)
                {
                        const T rnd = bd(engine) ? low_urd(engine) : high_urd(engine);
                        res.multiply_add(vectors[i], rnd);
                }
                return res;
        };

        std::vector<Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(random_point());
        }
        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> parallelotope_external_points(
        const Vector<N, T>& org,
        const Vector<N, T>& diagonal,
        const int count,
        RandomEngine& engine)
{
        std::uniform_real_distribution<T> low_urd(-10, -0.01);
        std::uniform_real_distribution<T> high_urd(1.01, 10);
        std::bernoulli_distribution bd(0.5);

        const auto random_point = [&]()
        {
                Vector<N, T> res = org;
                for (std::size_t i = 0; i < N; ++i)
                {
                        const T rnd = bd(engine) ? low_urd(engine) : high_urd(engine);
                        res[i] += diagonal[i] * rnd;
                }
                return res;
        };

        std::vector<Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(random_point());
        }
        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> parallelotope_internal_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd(0.01, 0.99);

        const auto random_point = [&]()
        {
                Vector<N, T> res = org;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res.multiply_add(vectors[i], urd(engine));
                }
                return res;
        };

        std::vector<Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(random_point());
        }
        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> parallelotope_internal_points(
        const Vector<N, T>& org,
        const Vector<N, T>& diagonal,
        const int count,
        RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd(0.01, 0.99);

        const auto random_point = [&]()
        {
                Vector<N, T> res = org;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] += diagonal[i] * urd(engine);
                }
                return res;
        };

        std::vector<Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(random_point());
        }
        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> parallelotope_cover_points(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors,
        const int count,
        RandomEngine& engine)
{
        std::uniform_real_distribution<T> cover_urd(-0.2, 1.2);
        std::uniform_real_distribution<T> len_urd(0, 1);

        const auto cover_point = [&]()
        {
                Vector<N, T> res = org;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res.multiply_add(vectors[i], cover_urd(engine));
                }
                return res;
        };

        const auto plane_point = [&](const std::size_t n)
        {
                Vector<N, T> res = org;
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (i != n)
                        {
                                res.multiply_add(vectors[i], len_urd(engine));
                        }
                }
                return res;
        };

        std::vector<Vector<N, T>> res;
        res.reserve(count * (1 + N * 2));
        for (int i = 0; i < count; ++i)
        {
                res.push_back(cover_point());
                for (std::size_t n = 0; n < N; ++n)
                {
                        res.push_back(plane_point(n));
                        res.push_back(vectors[n] + plane_point(n));
                }
        }
        return res;
}
}
