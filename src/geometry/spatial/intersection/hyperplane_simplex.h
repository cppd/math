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
#include <src/com/type/limit.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/geometry/spatial/random/vectors.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/sampling/simplex_uniform.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::geometry::spatial::intersection::hyperplane_simplex
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

template <std::size_t N, typename T>
struct Simplex final
{
        HyperplaneSimplex<N, T> simplex;
        std::array<numerical::Vector<N, T>, N> vertices;

        explicit Simplex(const std::array<numerical::Vector<N, T>, N>& vertices)
                : simplex(vertices),
                  vertices(vertices)
        {
        }
};

template <std::size_t N, typename T, typename RandomEngine>
Simplex<N, T> create_random_simplex(RandomEngine& engine)
{
        constexpr T ORG_INTERVAL = 10;
        constexpr T MIN_LENGTH = 0.1;
        constexpr T MAX_LENGTH = 10;

        const std::array<numerical::Vector<N, T>, N - 1> vectors =
                random::vectors<N - 1, N, T>(MIN_LENGTH, MAX_LENGTH, engine);
        const numerical::Vector<N, T> org = random::point<N, T>(ORG_INTERVAL, engine);

        std::array<numerical::Vector<N, T>, N> vertices;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                vertices[i] = org + vectors[i];
        }
        vertices[N - 1] = org;

        return Simplex(vertices);
}

template <std::size_t N, typename T>
T max_vertex_distance(const Simplex<N, T>& simplex)
{
        T max = Limits<T>::lowest();
        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = i + 1; j < N; ++j)
                {
                        max = std::max(max, (simplex.vertices[i] - simplex.vertices[j]).norm());
                }
        }
        return max;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<numerical::Ray<N, T>> create_rays(const Simplex<N, T>& simplex, const int point_count, RandomEngine& engine)
{
        const numerical::Vector<N, T>& normal = simplex.simplex.normal();

        const T distance = max_vertex_distance(simplex);

        const int ray_count = 3 * point_count;

        std::vector<numerical::Ray<N, T>> rays;
        rays.reserve(ray_count);

        for (int i = 0; i < point_count; ++i)
        {
                const numerical::Vector<N, T> point = sampling::uniform_in_simplex(engine, simplex.vertices);
                const numerical::Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray.moved(-1));
                rays.push_back(ray.moved(1).reversed());

                const numerical::Vector<N, T> direction = random::direction_for_normal(T{0}, T{0.5}, normal, engine);
                rays.push_back({ray.org() + distance * normal, -direction});
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));

        return rays;
}

template <std::size_t N, typename T>
void check_intersection_count(const Simplex<N, T>& simplex, const std::vector<numerical::Ray<N, T>>& rays)
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
                        if (simplex.simplex.intersect(ray))
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
        const Simplex<N, T> simplex = create_random_simplex<N, T>(engine);
        const std::vector<numerical::Ray<N, T>> rays = create_rays(simplex, point_count, engine);

        check_intersection_count(simplex, rays);

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const numerical::Ray<N, T>& ray : rays)
                {
                        do_not_optimize(simplex.simplex.intersect(ray));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

//

template <std::size_t N, typename T>
void test_intersection()
{
        PCG engine;

        const Simplex<N, T> simplex = create_random_simplex<N, T>(engine);
        const std::vector<numerical::Ray<N, T>> rays = create_rays(simplex, POINT_COUNT, engine);

        check_intersection_count(simplex, rays);
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
