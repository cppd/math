/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "average.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/geometry/spatial/hyperplane_ball.h>
#include <src/geometry/spatial/random/vectors.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace ns::geometry::spatial::intersection::hyperplane_ball
{
namespace implementation
{
inline constexpr int POINT_COUNT = 10'000;
inline constexpr int COMPUTE_COUNT = 100;
inline constexpr int AVERAGE_COUNT = 100;

template <typename T>
inline constexpr T ERROR_MIN = 0.998;
template <typename T>
inline constexpr T ERROR_MAX = 1.002;

template <std::size_t N, typename T, typename RandomEngine>
HyperplaneBall<N, T> create_random_hyperplane_ball(RandomEngine& engine)
{
        constexpr T ORG_INTERVAL = 10;
        constexpr T MIN_RADIUS = 0.1;
        constexpr T MAX_RADIUS = 5;

        return HyperplaneBall<N, T>(
                random::point<N, T>(ORG_INTERVAL, engine), sampling::uniform_on_sphere<N, T>(engine),
                std::uniform_real_distribution<T>(MIN_RADIUS, MAX_RADIUS)(engine));
}

template <std::size_t N, typename T>
std::array<numerical::Vector<N, T>, N - 1> ball_plane_vectors(const HyperplaneBall<N, T>& ball)
{
        const T radius = std::sqrt(ball.radius_squared());

        std::array<numerical::Vector<N, T>, N - 1> res = numerical::orthogonal_complement_of_unit_vector(ball.normal());
        for (numerical::Vector<N, T>& v : res)
        {
                v *= radius;
        }
        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<numerical::Ray<N, T>> create_rays(
        const HyperplaneBall<N, T>& ball,
        const int point_count,
        RandomEngine& engine)
{
        ASSERT(ball.normal().is_unit());

        const T distance = 2 * std::sqrt(ball.radius_squared());
        const std::array<numerical::Vector<N, T>, N - 1> vectors = ball_plane_vectors(ball);

        const int ray_count = 3 * point_count;

        std::vector<numerical::Ray<N, T>> rays;
        rays.reserve(ray_count);

        for (int i = 0; i < point_count; ++i)
        {
                const numerical::Vector<N, T> point = ball.center() + sampling::uniform_in_sphere(engine, vectors);
                const numerical::Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray.moved(-1));
                rays.push_back(ray.moved(1).reversed());

                const numerical::Vector<N, T> direction =
                        random::direction_for_normal(T{0}, T{0.5}, ball.normal(), engine);
                rays.push_back({ray.org() + distance * ball.normal(), -direction});
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));

        return rays;
}

template <std::size_t N, typename T>
void check_intersection_count(const HyperplaneBall<N, T>& ball, const std::vector<numerical::Ray<N, T>>& rays)
{
        if (!(rays.size() % 3 == 0))
        {
                error("Ray count " + to_string(rays.size()) + " is not a multiple of 3");
        }

        const std::size_t count = [&]
        {
                std::size_t res = 0;
                for (const numerical::Ray<N, T>& ray : rays)
                {
                        if (ball.intersect(ray))
                        {
                                ++res;
                        }
                }
                return res;
        }();

        const std::size_t expected_count = (rays.size() / 3) * 2;

        const T v = static_cast<T>(count) / expected_count;
        if (!(v >= ERROR_MIN<T> && v <= ERROR_MAX<T>))
        {
                error("Error intersection count " + to_string(count) + ", expected " + to_string(expected_count));
        }
}

template <std::size_t N, typename T, int COUNT, typename RandomEngine>
double compute_intersections_per_second(const int point_count, RandomEngine& engine)
{
        const HyperplaneBall<N, T> ball = create_random_hyperplane_ball<N, T>(engine);
        const std::vector<numerical::Ray<N, T>> rays = create_rays(ball, point_count, engine);

        check_intersection_count(ball, rays);

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const numerical::Ray<N, T>& ray : rays)
                {
                        do_not_optimize(ball.intersect(ray));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

//

template <std::size_t N, typename T>
void test_intersection()
{
        PCG engine;

        const HyperplaneBall<N, T> ball = create_random_hyperplane_ball<N, T>(engine);
        const std::vector<numerical::Ray<N, T>> rays = create_rays(ball, POINT_COUNT, engine);

        check_intersection_count(ball, rays);
}

template <std::size_t N, typename T>
double compute_intersections_per_second()
{
        PCG engine;

        return average<AVERAGE_COUNT>(
                [&]
                {
                        return compute_intersections_per_second<N, T, COMPUTE_COUNT>(POINT_COUNT, engine);
                });
}
}

template <std::size_t N, typename T>
void test_intersection()
{
        implementation::test_intersection<N, T>();
}

template <std::size_t N, typename T>
[[nodiscard]] double compute_intersections_per_second()
{
        return implementation::compute_intersections_per_second<N, T>();
}
}
