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

#include "../sphere_cosine.h"
#include "../testing/test.h"

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <sstream>

namespace ns::sampling::test
{
namespace
{
constexpr long long UNIT_COUNT = 10'000'000;
constexpr long long ANGLE_COUNT_PER_BUCKET = 1'000;
constexpr long long SURFACE_COUNT_PER_BUCKET = 10'000;
constexpr long long PERFORMANCE_COUNT = 10'000'000;

template <std::size_t N, typename T>
Vector<N, T> random_normal()
{
        PCG engine;
        return uniform_on_sphere<N, T>(engine).normalized();
}

//

template <std::size_t N, typename T>
void test_cosine_on_hemisphere(progress::Ratio* const progress)
{
        LOG("Sphere Cosine, " + space_name(N) + ", " + type_name<T>());

        const Vector<N, T> normal = random_normal<N, T>();

        testing::test_unit<N, T>(
                "", UNIT_COUNT,
                [&](auto& engine)
                {
                        return cosine_on_hemisphere(engine, normal);
                },
                progress);

        testing::test_distribution_angle<N, T>(
                "", ANGLE_COUNT_PER_BUCKET, normal,
                [&](auto& engine)
                {
                        return cosine_on_hemisphere(engine, normal);
                },
                [](const T angle)
                {
                        return cosine_on_hemisphere_pdf<N, T>(std::cos(angle));
                },
                progress);

        testing::test_distribution_surface<N, T>(
                "", SURFACE_COUNT_PER_BUCKET,
                [&](auto& engine)
                {
                        return cosine_on_hemisphere(engine, normal);
                },
                [&](const Vector<N, T>& v)
                {
                        return cosine_on_hemisphere_pdf<N, T>(dot(normal, v));
                },
                progress);

        testing::test_performance<PERFORMANCE_COUNT>(
                "",
                [&](auto& engine)
                {
                        return cosine_on_hemisphere(engine, normal);
                },
                progress);
}

template <std::size_t N>
void test_cosine_on_hemisphere(progress::Ratio* const progress)
{
        test_cosine_on_hemisphere<N, float>(progress);
        test_cosine_on_hemisphere<N, double>(progress);
}

//

template <std::size_t N, typename T>
void test_performance()
{
        const Vector<N, T> normal = random_normal<N, T>();

        const long long p = testing::test_performance<PERFORMANCE_COUNT>(
                [&](auto& engine)
                {
                        return cosine_on_hemisphere(engine, normal);
                });

        std::ostringstream oss;
        oss << "Sphere cosine <" << N << ", " << type_name<T>() << ">: ";
        oss << to_string_digit_groups(p) << " o/s";
        LOG(oss.str());
}

template <typename T, typename Counter>
void test_performance(const Counter& counter)
{
        counter();
        test_performance<3, T>();
        counter();
        test_performance<4, T>();
        counter();
        test_performance<5, T>();
}

void test_cosine_on_hemisphere_performance(progress::Ratio* const progress)
{
        constexpr int COUNT = 3 * 2;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };
        test_performance<float>(counter);
        test_performance<double>(counter);
}

//

TEST_LARGE("Sample Distribution, Sphere Cosine, 3-Space", test_cosine_on_hemisphere<3>)
TEST_LARGE("Sample Distribution, Sphere Cosine, 4-Space", test_cosine_on_hemisphere<4>)
TEST_LARGE("Sample Distribution, Sphere Cosine, 5-Space", test_cosine_on_hemisphere<5>)

TEST_PERFORMANCE("Sampling, Sphere Cosine", test_cosine_on_hemisphere_performance)
}
}
