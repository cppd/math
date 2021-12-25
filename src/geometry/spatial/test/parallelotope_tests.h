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

#include "../parallelotope.h"
#include "../parallelotope_aa.h"
#include "../testing/random_points.h"
#include "../testing/random_vectors.h"

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/random/pcg.h>

#include <array>
#include <cmath>
#include <random>

namespace ns::geometry::spatial::test
{
namespace parallelotope_test_implementation
{
inline constexpr bool PRINT = false;

inline void print_separator()
{
        if (PRINT)
        {
                LOG("---");
        }
}

inline void print_message(const std::string& msg)
{
        if (PRINT)
        {
                LOG(msg);
        }
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_direction(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd_dir(-1, 1);

        // equal probability is not needed
        while (true)
        {
                Vector<N, T> direction;
                for (std::size_t i = 0; i < N; ++i)
                {
                        direction[i] = urd_dir(engine);
                }
                if (direction.norm() > 0)
                {
                        return direction;
                }
        }
}

template <std::size_t N, std::size_t COUNT, typename T>
bool point_is_in_feasible_region(const Vector<N, T>& point, const std::array<Constraint<N, T>, COUNT>& c)
{
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                const T r = dot(c[i].a, point) + c[i].b;

                if (!std::isfinite(r))
                {
                        error("Not finite point " + to_string(point) + " and constraint a = " + to_string(c[i].a)
                              + ", b = " + to_string(c[i].b));
                }
                if (r < 0)
                {
                        return false;
                }
        }

        return true;
}

template <typename RandomEngine, typename Parallelotope>
void test_constraints(RandomEngine& engine, const int point_count, const Parallelotope& p)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        const Constraints<N, T, 2 * N, 0> constraints = p.constraints();

        for (const Vector<N, T>& point : testing::random_external_points(p.org(), p.vectors(), point_count, engine))
        {
                if (p.inside(point))
                {
                        error("Inside. Point must be outside\n" + to_string(point));
                }
                if (point_is_in_feasible_region(point, constraints.c))
                {
                        error("Constraints. Point must be outside\n" + to_string(point));
                }
        }

        for (const Vector<N, T>& origin : testing::random_internal_points(p.org(), p.vectors(), point_count, engine))
        {
                if (!p.inside(origin))
                {
                        error("Inside. Point must be inside\n" + to_string(origin));
                }
                if (!point_is_in_feasible_region(origin, constraints.c))
                {
                        error("Constraints. Point must be inside\n" + to_string(origin));
                }
        }
}

template <typename RandomEngine, typename Parallelotope>
void test_overlap(RandomEngine& engine, const int point_count, const Parallelotope& p)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        const T length = p.length();

        for (const Vector<N, T>& point : testing::random_internal_points(p.org(), p.vectors(), point_count, engine))
        {
                const Ray<N, T> ray(point, random_direction<N, T>(engine));

                {
                        const Ray<N, T> r = ray;
                        const auto t = p.intersect(r);
                        if (!t)
                        {
                                error("Ray must intersect\n" + to_string(r));
                        }
                        if (!(*t < length))
                        {
                                error("Intersection out of parallelotope.\ndistance = " + to_string(*t) + ", "
                                      + "max distance = " + to_string(length) + "\n" + to_string(r));
                        }
                }
                {
                        const Ray<N, T> r = ray.moved(-10 * length);
                        const auto t = p.intersect(r);
                        if (!t)
                        {
                                error("Ray must intersect\n" + to_string(r));
                        }
                }
                {
                        const Ray<N, T> r = ray.moved(10 * length);
                        const auto t = p.intersect(r);
                        if (t)
                        {
                                error("Ray must not intersect\n" + to_string(r));
                        }
                }
        }
}

template <std::size_t N, typename T>
void test_points(const int point_count)
{
        const std::string name = "Test parallelotope points in " + space_name(N);

        PCG engine;

        constexpr T ORG_INTERVAL = 10;
        constexpr T MIN_LENGTH = 0.1;
        constexpr T MAX_LENGTH = 20;

        LOG("------------------------------");
        LOG(name);

        print_separator();
        LOG("ParallelotopeAA");

        {
                Vector<N, T> org = testing::random_org<N, T>(ORG_INTERVAL, engine);
                std::array<T, N> edges = testing::random_aa_vectors<N, T>(MIN_LENGTH, MAX_LENGTH, engine);
                ParallelotopeAA<N, T> p(org, edges);

                print_message(to_string(p));

                test_constraints(engine, point_count, p);
                test_overlap(engine, point_count, p);
        }

        print_separator();
        LOG("Parallelotope");

        {
                Vector<N, T> org = testing::random_org<N, T>(ORG_INTERVAL, engine);
                std::array<Vector<N, T>, N> edges = testing::random_vectors<N, N, T>(MIN_LENGTH, MAX_LENGTH, engine);
                Parallelotope<N, T> p(org, edges);

                print_message(to_string(p));

                test_constraints(engine, point_count, p);
                test_overlap(engine, point_count, p);
        }

        print_separator();
        LOG("Parallelotope comparison");

        {
                Vector<N, T> org = testing::random_org<N, T>(ORG_INTERVAL, engine);
                std::array<T, N> edges = testing::random_aa_vectors<N, T>(MIN_LENGTH, MAX_LENGTH, engine);

                ParallelotopeAA<N, T> p_aa(org, edges);
                Parallelotope<N, T> p(org, edges);

                print_message("#1\n" + to_string(p_aa) + "\n#2\n" + to_string(p));

                compare_parallelotopes(engine, point_count, p_aa, p);
        }

        print_separator();
        LOG(name + " passed");
}

template <typename Parallelotope>
void test_algorithms(const Parallelotope& p)
{
        print_separator();
        print_message("length");
        print_message(to_string(p.length()));

        print_separator();
        print_message("vertices");
        for (auto v : p.vertices())
        {
                print_message(to_string(v));
        }

        if constexpr (Parallelotope::SPACE_DIMENSION <= 3)
        {
                print_separator();
                print_message("edges");
                for (auto edge : p.edges())
                {
                        print_message(to_string(edge));
                }
        }
}

template <std::size_t N, typename T>
void test_algorithms()
{
        const std::string name = "Test parallelotope algorithms in " + space_name(N);

        constexpr std::array<T, N> EDGES = make_array_value<T, N>(1);
        constexpr Vector<N, T> ORG(0);

        LOG("------------------------------");
        LOG(name);

        print_separator();
        LOG("ParallelotopeAA");

        {
                ParallelotopeAA<N, T> p(ORG, EDGES);
                test_algorithms(p);
        }

        print_separator();
        LOG("Parallelotope");

        {
                Parallelotope<N, T> p(ORG, EDGES);
                test_algorithms(p);
        }

        print_separator();
        LOG(name + " passed");
}
}

template <std::size_t N, typename T>
void test_points(const int point_count)
{
        parallelotope_test_implementation::test_points<N, T>(point_count);
}

template <std::size_t N, typename T>
void test_algorithms()
{
        parallelotope_test_implementation::test_algorithms<N, T>();
}
}
