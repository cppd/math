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

#include "../sphere_cosine.h"
#include "distribution/distribution.h"

#include <src/com/random/engine.h>
#include <src/test/test.h>

#include <cmath>
#include <random>
#include <string>

namespace ns::sampling::test
{
namespace
{
constexpr long long UNIT_COUNT = 10'000'000;
constexpr long long ANGLE_COUNT_PER_BUCKET = 1'000;
constexpr long long SURFACE_COUNT_PER_BUCKET = 10'000;
constexpr long long PERFORMANCE_COUNT = 10'000'000;

template <std::size_t N, typename T>
void test_cosine_on_hemisphere()
{
        const std::string name = "Cosine";

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                name, UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, normal);
                });

        test_distribution_angle<N, T>(
                name, ANGLE_COUNT_PER_BUCKET, normal,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, normal);
                },
                [](T angle)
                {
                        return cosine_on_hemisphere_pdf<N, T>(std::cos(angle));
                });

        test_distribution_surface<N, T>(
                name, SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, normal);
                },
                [&](const Vector<N, T>& v)
                {
                        return cosine_on_hemisphere_pdf<N, T>(dot(normal, v));
                });

        test_performance<N, T>(
                name, PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return cosine_on_hemisphere(random_engine, normal);
                });
}

template <std::size_t N, typename T>
void test_power_cosine_on_hemisphere()
{
        const T power = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return std::uniform_real_distribution<T>(1, 100)(random_engine);
        }();

        const std::string name = "Power Cosine, power = " + to_string_fixed(power, 1);

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                name, UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, normal, power);
                });

        test_distribution_angle<N, T>(
                name, ANGLE_COUNT_PER_BUCKET, normal,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, normal, power);
                },
                [&](T angle)
                {
                        return power_cosine_on_hemisphere_pdf<N, T>(std::cos(angle), power);
                });

        test_distribution_surface<N, T>(
                name, SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, normal, power);
                },
                [&](const Vector<N, T>& v)
                {
                        return power_cosine_on_hemisphere_pdf<N, T>(dot(normal, v), power);
                });

        test_performance<N, T>(
                name, PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return power_cosine_on_hemisphere(random_engine, normal, power);
                });
}

template <std::size_t N>
void test_cosine_on_hemisphere()
{
        test_cosine_on_hemisphere<N, float>();
        test_cosine_on_hemisphere<N, double>();
}

template <std::size_t N>
void test_power_cosine_on_hemisphere()
{
        test_power_cosine_on_hemisphere<N, float>();
        test_power_cosine_on_hemisphere<N, double>();
}

TEST_LARGE("Sample Distribution, Sphere Cosine, 3-Space", test_cosine_on_hemisphere<3>)
TEST_LARGE("Sample Distribution, Sphere Cosine, 4-Space", test_cosine_on_hemisphere<4>)
TEST_LARGE("Sample Distribution, Sphere Cosine, 5-Space", test_cosine_on_hemisphere<5>)

TEST_LARGE("Sample Distribution, Sphere Power Cosine, 3-Space", test_power_cosine_on_hemisphere<3>)
TEST_LARGE("Sample Distribution, Sphere Power Cosine, 4-Space", test_power_cosine_on_hemisphere<4>)
TEST_LARGE("Sample Distribution, Sphere Power Cosine, 5-Space", test_power_cosine_on_hemisphere<5>)
}
}
