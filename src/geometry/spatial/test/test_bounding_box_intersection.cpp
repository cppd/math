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
#include "bounding_box.h"
#include "parallelotope_points.h"

#include "../bounding_box.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::geometry::spatial::test
{
namespace
{
template <std::size_t N, typename T>
std::vector<Ray<N, T>> rays_for_intersections(
        const BoundingBox<N, T>& box,
        const int point_count,
        std::mt19937_64& engine)
{
        const T length = box.diagonal().norm();
        const T move_distance = length;
        const int ray_count = 3 * point_count;
        std::vector<Ray<N, T>> rays;
        rays.reserve(ray_count);
        for (const Vector<N, T>& point : internal_points(box.min(), bounding_box_vectors(box), point_count, engine))
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
void check_intersection_count(const BoundingBox<N, T>& box, const std::vector<Ray<N, T>>& rays)
{
        if (!(rays.size() % 3 == 0))
        {
                error("Ray count " + to_string(rays.size()) + " is not a multiple of 3");
        }
        std::size_t count = 0;
        for (const Ray<N, T>& ray : rays)
        {
                if (box.intersect(ray))
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

template <std::size_t N, typename T>
void check_intersection_count(
        const BoundingBox<N, T>& box,
        const std::vector<Vector<N, T>>& orgs,
        const std::vector<Vector<N, T>>& dirs_reciprocal,
        const std::vector<Vector<N, bool>>& dirs_negative)
{
        if (!(orgs.size() % 3 == 0))
        {
                error("Ray count " + to_string(orgs.size()) + " is not a multiple of 3");
        }
        if (orgs.size() != dirs_reciprocal.size() || orgs.size() != dirs_negative.size())
        {
                error("Ray data error");
        }
        std::size_t count = 0;
        for (std::size_t i = 0; i < orgs.size(); ++i)
        {
                if (box.intersect(orgs[i], dirs_reciprocal[i], dirs_negative[i]))
                {
                        ++count;
                }
        }
        const std::size_t expected_count = (orgs.size() / 3) * 2;
        if (count != expected_count)
        {
                error("Error intersection count " + to_string(count) + ", expected " + to_string(expected_count));
        }
}

template <std::size_t N, typename T>
std::vector<Vector<N, T>> ray_orgs(const std::vector<Ray<N, T>>& rays)
{
        std::vector<Vector<N, T>> res;
        res.reserve(rays.size());
        for (const Ray<N, T>& ray : rays)
        {
                res.push_back(ray.org());
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<Vector<N, T>> ray_reciprocal_directions(const std::vector<Ray<N, T>>& rays)
{
        std::vector<Vector<N, T>> res;
        res.reserve(rays.size());
        for (const Ray<N, T>& ray : rays)
        {
                res.push_back(reciprocal(ray.dir()));
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<Vector<N, bool>> ray_negative_directions(const std::vector<Ray<N, T>>& rays)
{
        std::vector<Vector<N, bool>> res;
        res.reserve(rays.size());
        for (const Ray<N, T>& ray : rays)
        {
                res.push_back(bounding_box_negative_directions(ray.dir()));
        }
        return res;
}

//

template <std::size_t N, typename T>
void test_intersection()
{
        constexpr int POINT_COUNT = 10'000;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const BoundingBox<N, T> box = create_random_bounding_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, POINT_COUNT, engine);

        check_intersection_count(box, rays);

        const std::vector<Vector<N, T>> orgs = ray_orgs(rays);
        const std::vector<Vector<N, T>> dirs_reciprocal = ray_reciprocal_directions(rays);
        const std::vector<Vector<N, bool>> dirs_negative = ray_negative_directions(rays);

        check_intersection_count(box, orgs, dirs_reciprocal, dirs_negative);
}

template <typename T>
void test_intersection()
{
        test_intersection<2, T>();
        test_intersection<3, T>();
        test_intersection<4, T>();
        test_intersection<5, T>();
}

void test_bounding_box_intersection()
{
        LOG("Test bounding box intersection");
        test_intersection<float>();
        test_intersection<double>();
        test_intersection<long double>();
        LOG("Test bounding box intersection passed");
}

//

template <std::size_t N, typename T, int COUNT>
double compute_intersections_per_second(const int point_count, std::mt19937_64& engine)
{
        const BoundingBox<N, T> box = create_random_bounding_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, point_count, engine);

        check_intersection_count(box, rays);

        Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Ray<N, T>& ray : rays)
                {
                        do_not_optimize(box.intersect(ray));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

template <std::size_t N, typename T, int COUNT>
double compute_intersections_r_per_second(const int point_count, std::mt19937_64& engine)
{
        const BoundingBox<N, T> box = create_random_bounding_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, point_count, engine);

        const std::vector<Vector<N, T>> orgs = ray_orgs(rays);
        const std::vector<Vector<N, T>> dirs_reciprocal = ray_reciprocal_directions(rays);
        const std::vector<Vector<N, bool>> dirs_negative = ray_negative_directions(rays);

        check_intersection_count(box, orgs, dirs_reciprocal, dirs_negative);

        const std::size_t n = rays.size();

        Clock::time_point start_time = Clock::now();
        for (int j = 0; j < COUNT; ++j)
        {
                for (std::size_t i = 0; i < n; ++i)
                {
                        do_not_optimize(box.intersect(orgs[i], dirs_reciprocal[i], dirs_negative[i]));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

template <std::size_t N, typename T>
double compute_intersections_per_second()
{
        constexpr int POINT_COUNT = 10'000;
        constexpr int COMPUTE_COUNT = 1000;
        constexpr int AVERAGE_COUNT = 10;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        return average<AVERAGE_COUNT>(
                [&]
                {
                        return compute_intersections_per_second<N, T, COMPUTE_COUNT>(POINT_COUNT, engine);
                });
}

template <std::size_t N, typename T>
double compute_intersections_r_per_second()
{
        constexpr int POINT_COUNT = 10'000;
        constexpr int COMPUTE_COUNT = 1000;
        constexpr int AVERAGE_COUNT = 10;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        return average<AVERAGE_COUNT>(
                [&]
                {
                        return compute_intersections_r_per_second<N, T, COMPUTE_COUNT>(POINT_COUNT, engine);
                });
}

template <std::size_t N, typename T>
void test_performance()
{
        const long long performance_1 = std::llround(compute_intersections_per_second<N, T>());
        const long long performance_2 = std::llround(compute_intersections_r_per_second<N, T>());

        LOG("BoundingBox<" + to_string(N) + ", " + type_name<T>() + ">: {" + to_string_digit_groups(performance_1)
            + ", " + to_string_digit_groups(performance_2) + "} i/s");
}

template <typename T>
void test_performance()
{
        test_performance<2, T>();
        test_performance<3, T>();
        test_performance<4, T>();
        test_performance<5, T>();
}

void test_bounding_box_performance()
{
        test_performance<float>();
        test_performance<double>();
}

//

TEST_SMALL("Bounding box intersection", test_bounding_box_intersection)
TEST_PERFORMANCE("Bounding box intersection", test_bounding_box_performance)
}
}
