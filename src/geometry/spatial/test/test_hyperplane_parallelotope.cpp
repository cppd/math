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
#include "generate.h"

#include "../hyperplane_parallelotope.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/sampling/parallelotope_uniform.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::geometry::spatial::test
{
namespace
{
template <std::size_t N, typename T>
HyperplaneParallelotope<N, T> create_random_hyperplane_parallelotope(std::mt19937_64& engine)
{
        constexpr T ORG_INTERVAL = 10;
        constexpr T MIN_LENGTH = 0.1;
        constexpr T MAX_LENGTH = 10;

        return HyperplaneParallelotope<N, T>(
                generate_org<N, T>(ORG_INTERVAL, engine),
                generate_vectors<N - 1, N, T>(MIN_LENGTH, MAX_LENGTH, engine));
}

template <std::size_t N, typename T>
Vector<N, T> random_vector(const T& min, const T& max, std::mt19937_64& engine)
{
        std::uniform_real_distribution<T> urd(min, max);
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = urd(engine);
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<Ray<N, T>> create_rays(
        const HyperplaneParallelotope<N, T>& p,
        const int point_count,
        std::mt19937_64& engine)
{
        std::uniform_real_distribution<T> urd(0, 1);

        const T distance = p.length();

        const int ray_count = 3 * point_count;
        std::vector<Ray<N, T>> rays;
        rays.reserve(ray_count);
        for (int i = 0; i < point_count; ++i)
        {
                const Vector<N - 1, T> sample = random_vector<N - 1, T>(0, 1, engine);
                const Vector<N, T> point = p.org() + sampling::uniform_in_parallelotope(p.vectors(), sample);
                const Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray.moved(-1));
                rays.push_back(ray.moved(1).reversed());

                const Vector<N, T> direction = generate_random_direction(T(0), T(0.5), p.normal(), engine);
                rays.push_back(Ray(ray.org() + distance * p.normal(), -direction));
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));
        return rays;
}

template <std::size_t N, typename T>
void check_intersection_count(const HyperplaneParallelotope<N, T>& p, const std::vector<Ray<N, T>>& rays)
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
        const T v = T(count) / expected_count;
        if (!(v >= T(0.999) && v <= T(1.001)))
        {
                error("Error intersection count " + to_string(count) + ", expected " + to_string(expected_count));
        }
}

//

template <std::size_t N, typename T>
void test()
{
        constexpr int POINT_COUNT = 10'000;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const HyperplaneParallelotope<N, T> p = create_random_hyperplane_parallelotope<N, T>(engine);
        const std::vector<Ray<N, T>> rays = create_rays(p, POINT_COUNT, engine);

        check_intersection_count(p, rays);
}

template <typename T>
void test()
{
        test<2, T>();
        test<3, T>();
        test<4, T>();
        test<5, T>();
}

void test_hyperplane_parallelotope()
{
        LOG("Test hyperplane parallelotope");
        test<float>();
        test<double>();
        test<long double>();
        LOG("Test hyperplane parallelotope passed");
}

//

template <std::size_t N, typename T, int COUNT>
double compute_intersections_per_second(const int point_count, std::mt19937_64& engine)
{
        const HyperplaneParallelotope<N, T> p = create_random_hyperplane_parallelotope<N, T>(engine);
        const std::vector<Ray<N, T>> rays = create_rays(p, point_count, engine);

        check_intersection_count(p, rays);

        Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Ray<N, T>& ray : rays)
                {
                        do_not_optimize(p.intersect(ray));
                }
        }
        return COUNT * (rays.size() / duration_from(start_time));
}

template <std::size_t N, typename T>
void test_performance()
{
        constexpr int POINT_COUNT = 10'000;
        constexpr int COMPUTE_COUNT = 1000;
        constexpr int AVERAGE_COUNT = 10;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const double performance = average<AVERAGE_COUNT>(
                [&]
                {
                        return compute_intersections_per_second<N, T, COMPUTE_COUNT>(POINT_COUNT, engine);
                });

        LOG("HyperplaneParallelotope<" + to_string(N) + ", " + type_name<T>() + ">, "
            + to_string_digit_groups(std::llround(performance)) + " intersections per second");
}

template <typename T>
void test_performance()
{
        test_performance<2, T>();
        test_performance<3, T>();
        test_performance<4, T>();
        test_performance<5, T>();
}

void test_hyperplane_parallelotope_performance()
{
        test_performance<float>();
        test_performance<double>();
}

//

TEST_SMALL("Hyperplane parallelotope", test_hyperplane_parallelotope)
TEST_PERFORMANCE("Hyperplane parallelotope intersection", test_hyperplane_parallelotope_performance)
}
}
