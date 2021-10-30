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

#include "average.h"
#include "random_points.h"
#include "random_vectors.h"

#include "../parallelotope.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::geometry::spatial::testing::parallelotope
{
namespace implementation
{
inline constexpr int POINT_COUNT = 10'000;
inline constexpr int COMPUTE_COUNT = 100;
inline constexpr int AVERAGE_COUNT = 100;

template <std::size_t N, typename T>
Parallelotope<N, T> create_random_parallelotope(std::mt19937_64& engine)
{
        constexpr T ORG_INTERVAL = 10;
        constexpr T MIN_LENGTH = 0.1;
        constexpr T MAX_LENGTH = 10;

        return Parallelotope<N, T>(
                random_org<N, T>(ORG_INTERVAL, engine), random_vectors<N, N, T>(MIN_LENGTH, MAX_LENGTH, engine));
}

template <std::size_t N, typename T>
std::vector<Ray<N, T>> create_rays(const Parallelotope<N, T>& p, const int point_count, std::mt19937_64& engine)
{
        const T move_distance = p.length();
        const int ray_count = 3 * point_count;
        std::vector<Ray<N, T>> rays;
        rays.reserve(ray_count);
        for (const Vector<N, T>& point : random_internal_points(p.org(), p.vectors(), point_count, engine))
        {
                const Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray);
                rays.push_back(ray.moved(-move_distance));
                rays.push_back(ray.moved(move_distance));
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));
        return rays;
}

template <std::size_t N, typename T>
void check_intersection_count(const Parallelotope<N, T>& p, const std::vector<Ray<N, T>>& rays)
{
        if (!(rays.size() % 3 == 0))
        {
                error("Ray count " + to_string(rays.size()) + " is not a multiple of 3");
        }
        std::size_t count = 0;
        for (const Ray<N, T>& ray : rays)
        {
                if (p.intersect(ray))
                {
                        ++count;
                }
        }
        const std::size_t expected_count = (rays.size() / 3) * 2;
        if (count != expected_count)
        {
                error("Error intersection count " + to_string(count) + ", expected " + to_string(expected_count));
        }
}

template <std::size_t N, typename T, int COUNT>
double compute_intersections_per_second(const int point_count, std::mt19937_64& engine)
{
        const Parallelotope<N, T> parallelotope = create_random_parallelotope<N, T>(engine);
        const std::vector<Ray<N, T>> rays = create_rays(parallelotope, point_count, engine);

        check_intersection_count(parallelotope, rays);

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Ray<N, T>& ray : rays)
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
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const Parallelotope<N, T> p = create_random_parallelotope<N, T>(engine);
        const std::vector<Ray<N, T>> rays = create_rays(p, POINT_COUNT, engine);

        check_intersection_count(p, rays);
}

template <std::size_t N, typename T>
double compute_intersections_per_second()
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

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

//

template <std::size_t N, typename T>
[[nodiscard]] T intersection_cost()
{
        static const T cost = 1 / compute_intersections_per_second<N, T>();
        return cost;
}
}
