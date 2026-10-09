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

#include "compare.h"

#include <src/com/log.h>
#include <src/com/random/pcg.h>
#include <src/numerical/vector_object.h>
#include <src/statistics/linear_fit.h>
#include <src/test/test.h>

#include <random>
#include <vector>

namespace ns::statistics::test
{
namespace
{
template <typename T>
void test(const T precision)
{
        const int count = 10'000;

        PCG engine;
        std::normal_distribution<T> nd(0, 0.1);

        std::vector<numerical::Vector<2, T>> points;
        points.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                points.push_back({i, i + nd(engine)});
        }

        const Line l = linear_fit(points);

        compare(l.intercept, T{0}, precision);
        compare(l.slope, T{1}, precision);
}

void test_median()
{
        LOG("Test linear fit");

        test<double>(1e-2);
        test<long double>(1e-2);

        LOG("Test linear fit passed");
}

TEST_SMALL("Linear Fit", test_median)
}
}
