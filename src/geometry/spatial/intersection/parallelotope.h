/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/geometry/spatial/parallelotope.h>
#include <src/geometry/spatial/random/parallelotope_points.h>
#include <src/geometry/spatial/random/vectors.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>

#include <cstddef>
#include <vector>

namespace ns::geometry::spatial::intersection::parallelotope
{
namespace implementation
{
inline constexpr int POINT_COUNT = 10'000;
inline constexpr int COMPUTE_COUNT = 100;
inline constexpr int AVERAGE_COUNT = 100;

template <std::size_t N, typename T, typename RandomEngine>
Parallelotope<N, T> create_random_parallelotope(RandomEngine& engine)
{
        constexpr T ORG_INTERVAL = 10;
        constexpr T MIN_LENGTH = 0.1;
        constexpr T MAX_LENGTH = 10;

        return Parallelotope<N, T>(
                random::point<N, T>(ORG_INTERVAL, engine), random::vectors<N, N, T>(MIN_LENGTH, MAX_LENGTH, engine));
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<numerical::Ray<N, T>> create_rays(const Parallelotope<N, T>& p, const int point_count, RandomEngine& engine)
{
        const T move_distance = p.length();
        const int ray_count = 3 * point_count;

        std::vector<numerical::Ray<N, T>> rays;
        rays.reserve(ray_count);

        for (const numerical::Vector<N, T>& point :
             random::parallelotope_internal_points(p.org(), p.vectors(), point_count, engine))
        {
                const numerical::Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray);
                rays.push_back(ray.moved(-move_distance));
                rays.push_back(ray.moved(move_distance));
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));

        return rays;
}

template <std::size_t N, typename T>
void check_intersection_count(const Parallelotope<N, T>& p, const std::vector<numerical::Ray<N, T>>& rays)
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
                        if (p.intersect(ray))
                        {
                                ++res;
                        }
                }
                return res;
        }();

        const std::size_t expected_count = (rays.size() / 3) * 2;

        if (count != expected_count)
        {
                error("Error intersection count " + to_string(count) + ", expected " + to_string(expected_count));
        }
}

template <std::size_t N, typename T, int COUNT, typename RandomEngine>
double compute_intersections_per_second(const int point_count, RandomEngine& engine)
{
        const Parallelotope<N, T> parallelotope = create_random_parallelotope<N, T>(engine);
        const std::vector<numerical::Ray<N, T>> rays = create_rays(parallelotope, point_count, engine);

        check_intersection_count(parallelotope, rays);

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const numerical::Ray<N, T>& ray : rays)
                {
                        do_not_optimize(parallelotope.intersect(ray));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

//

template <std::size_t N, typename T>
void test_intersection()
{
        PCG engine;

        const Parallelotope<N, T> p = create_random_parallelotope<N, T>(engine);
        const std::vector<numerical::Ray<N, T>> rays = create_rays(p, POINT_COUNT, engine);

        check_intersection_count(p, rays);
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
