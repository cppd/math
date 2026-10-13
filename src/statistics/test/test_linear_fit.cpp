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
void test_qr(PCG& engine)
{
        const T precision = 2e-2;
        const int count = 1'000;

        std::uniform_real_distribution<T> urd_intercept(-10, 10);
        std::uniform_real_distribution<T> urd_slope(-10, 10);

        const T intercept = urd_intercept(engine);
        const T slope = urd_slope(engine);

        std::normal_distribution<T> nd_y(0, 0.1);
        std::uniform_real_distribution<T> urd_x(-100, 100);

        std::vector<numerical::Vector<2, T>> points;
        points.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                const T x = urd_x(engine);
                const T y = intercept + slope * x + nd_y(engine);
                points.push_back({x, y});
        }

        const Line l = linear_fit_qr(points);

        compare(l.intercept, intercept, precision);
        compare(l.slope, slope, precision);
}

template <typename T>
void test_theil_sen(PCG& engine)
{
        const T precision = 4e-2;
        const int count = 1'000;

        std::uniform_real_distribution<T> urd_intercept(-10, 10);
        std::uniform_real_distribution<T> urd_slope(-10, 10);

        const T intercept = urd_intercept(engine);
        const T slope = urd_slope(engine);

        std::normal_distribution<T> nd_y(0, 0.1);
        std::bernoulli_distribution outlier(0.1);
        std::uniform_real_distribution<T> urd_outlier(0, 1000);
        std::uniform_real_distribution<T> urd_x(-100, 100);

        std::vector<numerical::Vector<2, T>> points;
        points.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                const T x = urd_x(engine);
                const T y = intercept + slope * x + (outlier(engine) ? urd_outlier(engine) : nd_y(engine));
                points.push_back({x, y});
        }

        const Line l = linear_fit_theil_sen(points);

        compare(l.intercept, intercept, precision);
        compare(l.slope, slope, precision);
}

template <typename T>
void test(PCG& engine)
{
        test_qr<T>(engine);
        test_theil_sen<T>(engine);
}

void test_median()
{
        LOG("Test linear fit");

        PCG engine;

        test<double>(engine);
        test<long double>(engine);

        LOG("Test linear fit passed");
}

TEST_SMALL("Linear Fit", test_median)
}
}
