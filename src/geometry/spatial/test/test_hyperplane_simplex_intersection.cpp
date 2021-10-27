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

#include "../testing/hyperplane_simplex_intersection.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <cmath>

namespace ns::geometry::spatial
{
namespace
{
template <typename T>
void test_intersection()
{
        testing::hyperplane_simplex::test_intersection<2, T>();
        testing::hyperplane_simplex::test_intersection<3, T>();
        testing::hyperplane_simplex::test_intersection<4, T>();
        testing::hyperplane_simplex::test_intersection<5, T>();
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
        const long long p = std::llround(testing::hyperplane_simplex::compute_intersections_per_second<N, T>());

        LOG("HyperplaneSimplex<" + to_string(N) + ", " + type_name<T>() + ">: " + to_string_digit_groups(p) + " i/s");
}

template <typename T, typename F>
void test_performance(const F& f)
{
        f();
        test_performance<2, T>();
        f();
        test_performance<3, T>();
        f();
        test_performance<4, T>();
        f();
        test_performance<5, T>();
}

void test_hyperplane_simplex_performance(ProgressRatio* const progress)
{
        constexpr int COUNT = 8;
        int i = -1;
        const auto f = [&]
        {
                progress->set(++i, COUNT);
        };
        test_performance<float>(f);
        test_performance<double>(f);
}

//

TEST_SMALL("Hyperplane simplex intersection", test_hyperplane_simplex_intersection)
TEST_PERFORMANCE("Hyperplane simplex intersection", test_hyperplane_simplex_performance)
}
}
