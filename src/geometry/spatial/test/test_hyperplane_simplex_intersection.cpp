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

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/name.h>
#include <src/geometry/spatial/intersection/hyperplane_simplex.h>
#include <src/progress/progress.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>

namespace ns::geometry::spatial::test
{
namespace
{
namespace test = intersection::hyperplane_simplex;

template <typename T>
void test_intersection()
{
        test::test_intersection<2, T>();
        test::test_intersection<3, T>();
        test::test_intersection<4, T>();
        test::test_intersection<5, T>();
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

template <std::size_t N, typename T>
void test_performance()
{
        const long long p = std::llround(test::compute_intersections_per_second<N, T>());

        LOG("HyperplaneSimplex<" + to_string(N) + ", " + type_name<T>() + ">: " + to_string_digit_groups(p) + " o/s");
}

template <typename T, typename Counter>
void test_performance(const Counter& counter)
{
        counter();
        test_performance<2, T>();
        counter();
        test_performance<3, T>();
        counter();
        test_performance<4, T>();
        counter();
        test_performance<5, T>();
}

void test_hyperplane_simplex_performance(progress::Ratio* const progress)
{
        constexpr int COUNT = 4 * 2;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };
        test_performance<float>(counter);
        test_performance<double>(counter);
}

//

TEST_SMALL("Hyperplane Simplex Intersection", test_hyperplane_simplex_intersection)
TEST_PERFORMANCE("Hyperplane Simplex Intersection", test_hyperplane_simplex_performance)
}
}
