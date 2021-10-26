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

#include "../hyperplane_simplex.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/complement.h>
#include <src/sampling/simplex_uniform.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::geometry::spatial::test
{
namespace
{

template <std::size_t N, typename T>
struct Simplex
{
        HyperplaneSimplex<N, T> simplex;
        Vector<N, T> normal;
        std::array<Vector<N, T>, N> vertices;
};

template <std::size_t N, typename T>
Simplex<N, T> create_random_simplex(std::mt19937_64& engine)
{
        constexpr T ORG_INTERVAL = 10;
        constexpr T MIN_LENGTH = 0.1;
        constexpr T MAX_LENGTH = 10;

        const std::array<Vector<N, T>, N - 1> vectors = generate_vectors<N - 1, N, T>(MIN_LENGTH, MAX_LENGTH, engine);
        const Vector<N, T> org = generate_org<N, T>(ORG_INTERVAL, engine);

        Simplex<N, T> simplex;

        simplex.normal = numerical::orthogonal_complement(vectors).normalized();
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                simplex.vertices[i] = org + vectors[i];
        }
        simplex.vertices[N - 1] = org;
        simplex.simplex.set_data(simplex.normal, simplex.vertices);

        return simplex;
}

template <std::size_t N, typename T>
T max_vertex_distance(const std::array<Vector<N, T>, N>& vertices)
{
        T max = Limits<T>::lowest();
        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = i + 1; j < N; ++j)
                {
                        max = std::max(max, (vertices[i] - vertices[j]).norm());
                }
        }
        return max;
}

template <std::size_t N, typename T>
std::vector<Ray<N, T>> create_rays(
        const Vector<N, T>& normal,
        const std::array<Vector<N, T>, N>& vertices,
        const int point_count,
        std::mt19937_64& engine)
{
        const T distance = max_vertex_distance(vertices);

        const int ray_count = 3 * point_count;
        std::vector<Ray<N, T>> rays;
        rays.reserve(ray_count);
        for (int i = 0; i < point_count; ++i)
        {
                const Vector<N, T> point = sampling::uniform_in_simplex(vertices, engine);
                const Ray<N, T> ray(point, sampling::uniform_on_sphere<N, T>(engine));
                rays.push_back(ray.moved(-1));
                rays.push_back(ray.moved(1).reversed());

                const Vector<N, T> direction = generate_random_direction(T(0), T(0.5), normal, engine);
                rays.push_back(Ray(ray.org() + distance * normal, -direction));
        }
        ASSERT(rays.size() == static_cast<std::size_t>(ray_count));
        return rays;
}

template <std::size_t N, typename T>
void check_intersection_count(const Simplex<N, T>& simplex, const std::vector<Ray<N, T>>& rays)
{
        if (!(rays.size() % 3 == 0))
        {
                error("Ray count " + to_string(rays.size()) + " is not a multiple of 3");
        }

        std::size_t count = 0;
        for (const Ray<N, T>& ray : rays)
        {
                if (simplex.simplex.intersect(ray, simplex.vertices[0], simplex.normal))
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
void test_intersection()
{
        constexpr int POINT_COUNT = 10'000;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const Simplex<N, T> simplex = create_random_simplex<N, T>(engine);
        const std::vector<Ray<N, T>> rays = create_rays(simplex.normal, simplex.vertices, POINT_COUNT, engine);

        check_intersection_count(simplex, rays);
}

template <typename T>
void test_intersection()
{
        test_intersection<2, T>();
        test_intersection<3, T>();
        test_intersection<4, T>();
        test_intersection<5, T>();
}

void test_hyperplane_simplex_intersection()
{
        LOG("Test hyperplane simplex intersection");
        test_intersection<float>();
        test_intersection<double>();
        test_intersection<long double>();
        LOG("Test hyperplane simplex intersection passed");
}

//

template <std::size_t N, typename T, int COUNT>
double compute_intersections_per_second(const int point_count, std::mt19937_64& engine)
{
        const Simplex<N, T> simplex = create_random_simplex<N, T>(engine);
        const std::vector<Ray<N, T>> rays = create_rays(simplex.normal, simplex.vertices, point_count, engine);

        check_intersection_count(simplex, rays);

        Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Ray<N, T>& ray : rays)
                {
                        do_not_optimize(simplex.simplex.intersect(ray, simplex.vertices[0], simplex.normal));
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
void test_performance()
{
        const long long performance = std::llround(compute_intersections_per_second<N, T>());

        LOG("HyperplaneSimplex<" + to_string(N) + ", " + type_name<T>() + ">: " + to_string_digit_groups(performance)
            + " i/s");
}

template <typename T>
void test_performance()
{
        test_performance<2, T>();
        test_performance<3, T>();
        test_performance<4, T>();
        test_performance<5, T>();
}

void test_hyperplane_simplex_performance()
{
        test_performance<float>();
        test_performance<double>();
}

//

TEST_SMALL("Hyperplane simplex intersection", test_hyperplane_simplex_intersection)
TEST_PERFORMANCE("Hyperplane simplex intersection", test_hyperplane_simplex_performance)
}
}
