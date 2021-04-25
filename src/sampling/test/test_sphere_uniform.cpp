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

#include "../sphere_uniform.h"
#include "distribution/distribution.h"

#include <src/com/random/engine.h>
#include <src/test/test.h>

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
void test_sphere_uniform()
{
        const std::string name = "Uniform";

        const Vector<N, T> normal = []()
        {
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                return uniform_on_sphere<N, T>(random_engine).normalized();
        }();

        test_unit<N, T>(
                name, UNIT_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                });

        test_distribution_angle<N, T>(
                name, ANGLE_COUNT_PER_BUCKET, normal,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                },
                [](T /*angle*/)
                {
                        return uniform_on_sphere_pdf<N, T>();
                });

        test_distribution_surface<N, T>(
                name, SURFACE_COUNT_PER_BUCKET,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                },
                [&](const Vector<N, T>& /*v*/)
                {
                        return uniform_on_sphere_pdf<N, T>();
                });

        test_performance<N, T>(
                name, PERFORMANCE_COUNT,
                [&](RandomEngine<T>& random_engine)
                {
                        return uniform_on_sphere<N, T>(random_engine);
                });
}

template <std::size_t N>
void test_sphere_uniform()
{
        test_sphere_uniform<N, float>();
        test_sphere_uniform<N, double>();
}

TEST_LARGE("Sample Distribution, Sphere Uniform, 3-Space", test_sphere_uniform<3>)
TEST_LARGE("Sample Distribution, Sphere Uniform, 4-Space", test_sphere_uniform<4>)
TEST_LARGE("Sample Distribution, Sphere Uniform, 5-Space", test_sphere_uniform<5>)
}
}
