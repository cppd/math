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

#include "../random/parallelotope_points.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <tuple>

namespace ns::geometry::spatial::test
{
namespace compare_parallelotopes_implementation
{
template <typename T>
inline constexpr T EQUALITY_EPSILON;
template <>
inline constexpr double EQUALITY_EPSILON<float> = 1e-5;
template <>
inline constexpr double EQUALITY_EPSILON<double> = 1e-14;

template <typename T>
bool equal(const T& a, const T& b)
{
        return std::abs(a - b) <= EQUALITY_EPSILON<T>;
}

template <std::size_t N, typename T>
bool equal(const Vector<N, T>& a, const Vector<N, T>& b)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!equal(a[i], b[i]))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_direction(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd_dir(-1, 1);
        std::uniform_int_distribution<int> uid_dir(-1, 1);
        std::uniform_int_distribution<int> uid_select(0, 10);

        // equal probability is not needed
        while (true)
        {
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = uid_select(engine) != 0 ? urd_dir(engine) : uid_dir(engine);
                }
                if (res.norm() > 0)
                {
                        return res;
                }
        }
}

template <std::size_t N, typename T, typename... Parallelotope>
void compare_intersections(const Ray<N, T>& ray, const Parallelotope&... p)
{
        static_assert(((N == Parallelotope::SPACE_DIMENSION) && ...));
        static_assert(((std::is_same_v<T, typename Parallelotope::DataType>)&&...));

        std::array<std::optional<T>, sizeof...(Parallelotope)> intersections{p.intersect(ray)...};

        const auto& first = intersections[0];

        for (std::size_t i = 1; i < sizeof...(Parallelotope); ++i)
        {
                const auto& intersection = intersections[i];
                if (intersection.has_value() != first.has_value())
                {
                        error("Error intersection comparison\n" + to_string(ray));
                }
                if (!intersection)
                {
                        continue;
                }
                if (!equal(*intersection, *first))
                {
                        std::string s = "Error intersection distance comparison.\n";
                        s += "Distance[" + to_string(i) + "] = " + to_string(*intersection) + "\n";
                        s += "Distance[0] = " + to_string(*first) + "\n";
                        s += "Ray = " + to_string(ray);
                        error(s);
                }
        }
}

template <std::size_t N, typename T, std::size_t COUNT>
void compare_vectors(const std::array<Vector<N, T>, COUNT>& vectors, const std::string& name)
{
        for (std::size_t i = 1; i < COUNT; ++i)
        {
                if (!equal(vectors[i], vectors[0]))
                {
                        error("Error comparison of " + name + ".\n" + to_string(vectors[i]) + " and "
                              + to_string(vectors[0]));
                }
        }
}

template <std::size_t N, typename T, std::size_t COUNT>
void compare_vectors(const std::array<std::array<Vector<N, T>, N>, COUNT>& vectors, const std::string& name)
{
        for (std::size_t i = 1; i < COUNT; ++i)
        {
                for (std::size_t n = 0; n < N; ++n)
                {
                        if (!equal(vectors[i][n], vectors[0][n]))
                        {
                                error("Error comparison of " + name + ".\n" + to_string(vectors[i][n]) + " and "
                                      + to_string(vectors[0][n]));
                        }
                }
        }
}

template <typename RandomEngine, typename... Parallelotope>
void compare_parallelotopes(RandomEngine& engine, const int point_count, const Parallelotope&... p)
{
        static_assert(sizeof...(Parallelotope) >= 2);

        constexpr std::size_t N = std::get<0>(std::make_tuple(Parallelotope::SPACE_DIMENSION...));
        using T = std::tuple_element_t<0, std::tuple<Parallelotope...>>::DataType;

        static_assert(((N == Parallelotope::SPACE_DIMENSION) && ...));
        static_assert(((std::is_same_v<T, typename Parallelotope::DataType>)&&...));

        std::array<T, sizeof...(Parallelotope)> lengths{p.length()...};

        for (std::size_t i = 1; i < sizeof...(Parallelotope); ++i)
        {
                if (!equal(lengths[i], lengths[0]))
                {
                        error("Error diagonal max length.\n" + to_string(lengths[i]) + " and " + to_string(lengths[0]));
                }
        }

        const std::array<Vector<N, T>, sizeof...(Parallelotope)> orgs{p.org()...};
        compare_vectors(orgs, "orgs");

        const std::array<std::array<Vector<N, T>, N>, sizeof...(Parallelotope)> vectors{p.vectors()...};
        compare_vectors(vectors, "vectors");

        const auto& parallelotope = *std::get<0>(std::make_tuple(&p...));

        for (const Vector<N, T>& point :
             random::parallelotope_cover_points(parallelotope.org(), parallelotope.vectors(), point_count, engine))
        {
                std::array<bool, sizeof...(Parallelotope)> inside{p.inside(point)...};

                for (std::size_t i = 1; i < sizeof...(Parallelotope); ++i)
                {
                        if (inside[i] != inside[0])
                        {
                                error("Error point inside\n" + to_string(point));
                        }
                }

                const Ray<N, T> ray(point, random_direction<N, T>(engine));

                compare_intersections(ray, p...);
                compare_intersections(ray.moved(-10 * lengths[0]), p...);
                compare_intersections(ray.moved(10 * lengths[0]), p...);
                compare_intersections(ray.moved(10 * lengths[0]).reversed(), p...);
                compare_intersections(ray.moved(-10 * lengths[0]).reversed(), p...);
        }
}
}

template <typename RandomEngine, typename... Parallelotope>
void compare_parallelotopes(RandomEngine& engine, const int point_count, Parallelotope&&... p)
{
        namespace impl = compare_parallelotopes_implementation;

        impl::compare_parallelotopes(engine, point_count, std::forward<Parallelotope>(p)...);
}
}
