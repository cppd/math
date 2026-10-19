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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/random/pcg.h>
#include <src/numerical/vector_object.h>
#include <src/statistics/linear_fit.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace ns::statistics::test
{
namespace
{
template <typename T>
void compare_less(const T a, const T b)
{
        if (!(a < b))
        {
                error(to_string(a) + " is not less than " + to_string(b));
        }
}

template <typename T>
T piecewise_function(const T& x, const std::vector<T>& betas, const std::vector<T>& breakpoints)
{
        ASSERT(betas.size() == breakpoints.size() + 1);

        T res = betas[0];
        for (std::size_t b = 0; b < breakpoints.size(); ++b)
        {
                if (x > breakpoints[b])
                {
                        res += betas[b + 1] * (x - breakpoints[b]);
                }
        }
        return res;
}

template <typename T>
T compute_rmse(
        const std::vector<numerical::Vector<2, T>>& points,
        const T& intercept,
        const T& slope,
        const T& s_intercept,
        const T& s_slope)
{
        T sum = 0;
        for (const numerical::Vector<2, T>& p : points)
        {
                const T y = intercept + slope * p[0];
                const T sy = s_intercept + s_slope * p[0];
                const T d = y - sy;
                sum += d * d;
        }
        return std::sqrt(sum / points.size());
}

template <typename T>
T compute_rmse(
        const std::vector<numerical::Vector<2, T>>& points,
        const std::vector<T>& breakpoints,
        const std::vector<T>& betas,
        const std::vector<T>& s_betas)
{
        T sum = 0;
        for (const numerical::Vector<2, T>& p : points)
        {
                const T x = p[0];
                const T y = piecewise_function(x, betas, breakpoints);
                const T sy = piecewise_function(x, s_betas, breakpoints);
                const T d = y - sy;
                sum += d * d;
        }
        return std::sqrt(sum / points.size());
}

template <typename T>
void test_qr(PCG& engine)
{
        const T precision = 2e-2;
        const int count = 1'000;
        const T noise_stddev = 0.1;

        std::uniform_real_distribution<T> urd_intercept(-10, 10);
        std::uniform_real_distribution<T> urd_slope(-10, 10);

        const T intercept = urd_intercept(engine);
        const T slope = urd_slope(engine);

        std::normal_distribution<T> nd_y(0, noise_stddev);
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

        const T rmse = compute_rmse(points, intercept, slope, l.intercept, l.slope);

        compare_less(rmse, 5 * noise_stddev / std::sqrt(count));
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
void test_linear_piecewise(PCG& engine)
{
        const T precision = 6e-2;
        const int count = 1'000;
        const T noise_stddev = 0.1;

        std::uniform_real_distribution<T> urd_betas(-10, 10);
        const std::vector<T> betas{
                urd_betas(engine), urd_betas(engine), urd_betas(engine), urd_betas(engine), urd_betas(engine)};

        std::uniform_real_distribution<T> urd_breakpoint1(-51, -49);
        std::uniform_real_distribution<T> urd_breakpoint2(-1, 1);
        std::uniform_real_distribution<T> urd_breakpoint3(49, 51);
        const std::vector<T> breakpoints{
                -100, urd_breakpoint1(engine), urd_breakpoint2(engine), urd_breakpoint3(engine)};

        std::uniform_real_distribution<T> urd_x(-100, 100);
        std::normal_distribution<T> nd_y(0, noise_stddev);

        std::vector<numerical::Vector<2, T>> points;
        points.reserve(count);

        for (int i = 0; i < count; ++i)
        {
                const T x = urd_x(engine);
                const T y = piecewise_function(x, betas, breakpoints) + nd_y(engine);
                points.push_back({x, y});
        }

        const std::vector<T> betas_s = linear_fit_piecewise(points, breakpoints);

        for (std::size_t i = 0; i < betas.size(); ++i)
        {
                compare(betas_s[i], betas[i], precision);
        }

        const T rmse = compute_rmse(points, breakpoints, betas, betas_s);

        compare_less(rmse, 5 * noise_stddev / std::sqrt(count));
}

template <typename T>
void test(PCG& engine)
{
        test_qr<T>(engine);
        test_theil_sen<T>(engine);
        test_linear_piecewise<T>(engine);
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
