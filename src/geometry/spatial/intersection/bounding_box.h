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

#include "average.h"

#include "../bounding_box.h"
#include "../random/parallelotope_points.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/sampling/sphere_uniform.h>

#include <cmath>
#include <random>

namespace ns::geometry::spatial::intersection::bounding_box
{
namespace implementation
{
inline constexpr int POINT_COUNT = 10'000;
inline constexpr int COMPUTE_COUNT = 100;
inline constexpr int AVERAGE_COUNT = 100;

template <std::size_t N, typename T, typename RandomEngine>
BoundingBox<N, T> create_random_bounding_box(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd(-5, 5);
        Vector<N, T> p1;
        Vector<N, T> p2;
        for (std::size_t i = 0; i < N; ++i)
        {
                do
                {
                        p1[i] = urd(engine);
                        p2[i] = urd(engine);
                } while (!(std::abs(p1[i] - p2[i]) >= T{0.5}));
        }
        return {p1, p2};
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Ray<N, T>> rays_for_intersections(const BoundingBox<N, T>& box, const int point_count, RandomEngine& engine)
{
        const T length = box.diagonal().norm();
        const T move_distance = length;
        const int ray_count = 3 * point_count;

        std::vector<Ray<N, T>> rays;
        rays.reserve(ray_count);

        for (const Vector<N, T>& point :
             random::parallelotope_internal_points(box.min(), box.diagonal(), point_count, engine))
        {
                const Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray);
                rays.push_back(ray.moved(-move_distance));
                rays.push_back(ray.moved(move_distance));
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));

        return rays;
}

template <bool VOLUME, std::size_t N, typename T>
std::size_t intersection_count(const BoundingBox<N, T>& box, const std::vector<Ray<N, T>>& rays)
{
        std::size_t res = 0;
        for (const Ray<N, T>& ray : rays)
        {
                if constexpr (!VOLUME)
                {
                        if (box.intersect(ray))
                        {
                                ++res;
                        }
                }
                else
                {
                        if (box.intersect_volume(ray))
                        {
                                ++res;
                        }
                }
        }
        return res;
}

template <bool VOLUME, std::size_t N, typename T>
void check_intersection_count(const BoundingBox<N, T>& box, const std::vector<Ray<N, T>>& rays)
{
        if (!(rays.size() % 3 == 0))
        {
                error("Ray count " + to_string(rays.size()) + " is not a multiple of 3");
        }

        const std::size_t count = intersection_count<VOLUME>(box, rays);

        const std::size_t expected_count = (rays.size() / 3) * 2;

        if (count != expected_count)
        {
                error("Error intersection count " + to_string(count) + ", expected " + to_string(expected_count));
        }
}

template <std::size_t N, typename T>
std::size_t intersection_count(
        const BoundingBox<N, T>& box,
        const std::vector<Vector<N, T>>& orgs,
        const std::vector<Vector<N, T>>& dirs_reciprocal,
        const std::vector<Vector<N, bool>>& dirs_negative)
{
        std::size_t res = 0;
        for (std::size_t i = 0; i < orgs.size(); ++i)
        {
                if (box.intersect(orgs[i], dirs_reciprocal[i], dirs_negative[i]))
                {
                        ++res;
                }
        }
        return res;
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

        const std::size_t count = intersection_count(box, orgs, dirs_reciprocal, dirs_negative);

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
                res.push_back(ray.dir().reciprocal());
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
                res.push_back(ray.dir().negative_bool());
        }
        return res;
}

template <std::size_t N, typename T, int COUNT, typename RandomEngine>
double compute_intersections_per_second(const int point_count, RandomEngine& engine)
{
        const BoundingBox<N, T> box = create_random_bounding_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, point_count, engine);

        check_intersection_count</*volume*/ false>(box, rays);

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Ray<N, T>& ray : rays)
                {
                        do_not_optimize(box.intersect(ray));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

template <std::size_t N, typename T, int COUNT, typename RandomEngine>
double compute_volume_intersections_per_second(const int point_count, RandomEngine& engine)
{
        const BoundingBox<N, T> box = create_random_bounding_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, point_count, engine);

        check_intersection_count</*volume*/ true>(box, rays);

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Ray<N, T>& ray : rays)
                {
                        do_not_optimize(box.intersect_volume(ray));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

template <std::size_t N, typename T, int COUNT, typename RandomEngine>
double compute_intersections_r_per_second(const int point_count, RandomEngine& engine)
{
        const BoundingBox<N, T> box = create_random_bounding_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, point_count, engine);

        const std::vector<Vector<N, T>> orgs = ray_orgs(rays);
        const std::vector<Vector<N, T>> dirs_reciprocal = ray_reciprocal_directions(rays);
        const std::vector<Vector<N, bool>> dirs_negative = ray_negative_directions(rays);

        check_intersection_count(box, orgs, dirs_reciprocal, dirs_negative);

        const std::size_t ray_count = rays.size();

        const Clock::time_point start_time = Clock::now();
        for (int count = 0; count < COUNT; ++count)
        {
                for (std::size_t i = 0; i < ray_count; ++i)
                {
                        do_not_optimize(box.intersect(orgs[i], dirs_reciprocal[i], dirs_negative[i]));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

//

template <std::size_t N, typename T>
void test_intersection()
{
        PCG engine;

        const BoundingBox<N, T> box = create_random_bounding_box<N, T>(engine);
        const std::vector<Ray<N, T>> rays = rays_for_intersections(box, POINT_COUNT, engine);

        check_intersection_count</*volume*/ false>(box, rays);
        check_intersection_count</*volume*/ true>(box, rays);

        const std::vector<Vector<N, T>> orgs = ray_orgs(rays);
        const std::vector<Vector<N, T>> dirs_reciprocal = ray_reciprocal_directions(rays);
        const std::vector<Vector<N, bool>> dirs_negative = ray_negative_directions(rays);

        check_intersection_count(box, orgs, dirs_reciprocal, dirs_negative);
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

template <std::size_t N, typename T>
double compute_volume_intersections_per_second()
{
        PCG engine;

        return average<AVERAGE_COUNT>(
                [&]
                {
                        return compute_volume_intersections_per_second<N, T, COMPUTE_COUNT>(POINT_COUNT, engine);
                });
}

template <std::size_t N, typename T>
double compute_intersections_r_per_second()
{
        PCG engine;

        return average<AVERAGE_COUNT>(
                [&]
                {
                        return compute_intersections_r_per_second<N, T, COMPUTE_COUNT>(POINT_COUNT, engine);
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

template <std::size_t N, typename T>
[[nodiscard]] double compute_volume_intersections_per_second()
{
        return implementation::compute_volume_intersections_per_second<N, T>();
}

template <std::size_t N, typename T>
[[nodiscard]] double compute_intersections_r_per_second()
{
        return implementation::compute_intersections_r_per_second<N, T>();
}
}
