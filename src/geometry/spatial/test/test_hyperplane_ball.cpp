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

#include "../hyperplane_ball.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/numerical/complement.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::geometry::spatial::test
{
namespace
{
template <std::size_t N, typename T>
HyperplaneBall<N, T> create_random_hyperplane_ball(std::mt19937_64& engine)
{
        constexpr T ORG_INTERVAL = 10;
        constexpr T MIN_RADIUS = 0.1;
        constexpr T MAX_RADIUS = 5;

        return HyperplaneBall<N, T>(
                generate_org<N, T>(ORG_INTERVAL, engine), sampling::uniform_on_sphere<N, T>(engine),
                std::uniform_real_distribution<T>(MIN_RADIUS, MAX_RADIUS)(engine));
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N - 1> ball_vectors(const HyperplaneBall<N, T>& ball)
{
        const T radius = std::sqrt(ball.radius_squared());
        std::array<Vector<N, T>, N - 1> vectors = numerical::orthogonal_complement_of_unit_vector(ball.normal());
        for (Vector<N, T>& v : vectors)
        {
                v *= radius;
        }
        return vectors;
}

template <std::size_t N, typename T>
std::vector<Ray<N, T>> create_rays(const HyperplaneBall<N, T>& ball, const int point_count, std::mt19937_64& engine)
{
        ASSERT(ball.normal().is_unit());

        const T distance = 2 * std::sqrt(ball.radius_squared());
        const std::array<Vector<N, T>, N - 1> vectors = ball_vectors(ball);

        const int ray_count = 3 * point_count;
        std::vector<Ray<N, T>> rays;
        rays.reserve(ray_count);
        for (int i = 0; i < point_count; ++i)
        {
                const Vector<N, T> point = ball.center() + sampling::uniform_in_sphere(vectors, engine);
                const Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray.moved(-1));
                rays.push_back(ray.moved(1).reversed());

                const Vector<N, T> direction = generate_random_direction(T(0), T(0.5), ball.normal(), engine);
                rays.push_back(Ray(ray.org() + distance * ball.normal(), -direction));
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));
        return rays;
}

template <std::size_t N, typename T>
void check_intersection_count(const HyperplaneBall<N, T>& ball, const std::vector<Ray<N, T>>& rays)
{
        if (!(rays.size() % 3 == 0))
        {
                error("Ray count " + to_string(rays.size()) + " is not a multiple of 3");
        }

        std::size_t count = 0;
        for (const Ray<N, T>& ray : rays)
        {
                if (ball.intersect(ray))
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

        const HyperplaneBall<N, T> ball = create_random_hyperplane_ball<N, T>(engine);
        const std::vector<Ray<N, T>> rays = create_rays(ball, POINT_COUNT, engine);

        check_intersection_count(ball, rays);
}

template <typename T>
void test()
{
        test<3, T>();
        test<4, T>();
        test<5, T>();
}

void test_hyperplane_ball()
{
        LOG("Test hyperplane ball");
        test<float>();
        test<double>();
        test<long double>();
        LOG("Test hyperplane ball passed");
}

//

template <std::size_t N, typename T, int COUNT>
double compute_intersections_per_second(const int point_count, std::mt19937_64& engine)
{
        const HyperplaneBall<N, T> ball = create_random_hyperplane_ball<N, T>(engine);
        const std::vector<Ray<N, T>> rays = create_rays(ball, point_count, engine);

        check_intersection_count(ball, rays);

        Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Ray<N, T>& ray : rays)
                {
                        do_not_optimize(ball.intersect(ray));
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

        LOG("HyperplaneBall<" + to_string(N) + ", " + type_name<T>() + ">, "
            + to_string_digit_groups(std::llround(performance)) + " intersections per second");
}

template <typename T>
void test_performance()
{
        test_performance<3, T>();
        test_performance<4, T>();
        test_performance<5, T>();
}

void test_hyperplane_ball_performance()
{
        test_performance<float>();
        test_performance<double>();
}

//

TEST_SMALL("Hyperplane ball", test_hyperplane_ball)
TEST_PERFORMANCE("Hyperplane ball intersection", test_hyperplane_ball_performance)
}
}