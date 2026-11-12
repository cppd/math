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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/random/pcg.h>
#include <src/numerical/vector_object.h>
#include <src/statistics/linear_fit/linear_fit.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace ns::statistics::test
{
namespace
{
#define COMPARE_LESS(a, b)                                                                                            \
        do                                                                                                            \
        {                                                                                                             \
                if (!((a) < (b)))                                                                                     \
                {                                                                                                     \
                        error(std::string(__func__) + ": " + to_string((a)) + " is not less than " + to_string((b))); \
                }                                                                                                     \
        } while (false)

template <typename T>
T piecewise_function(const T x, const std::vector<T>& betas, const std::vector<T>& breakpoints)
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
        const linear_fit::Line<T>& l1,
        const linear_fit::Line<T>& l2)
{
        T sum = 0;
        for (const numerical::Vector<2, T>& p : points)
        {
                const T y1 = l1.intercept + l1.slope * p[0];
                const T y2 = l2.intercept + l2.slope * p[0];
                const T d = y1 - y2;
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
T expected_stddev(const T noise_stddev, const int count)
{
        return noise_stddev / std::sqrt(count);
}

template <typename T>
T expected_stddev(const T noise_stddev, const int count, const T outlier_probability)
{
        return noise_stddev / std::sqrt((1 - outlier_probability) * count);
}

template <typename T>
linear_fit::Line<T> random_line(PCG& engine)
{
        const T intercept = std::uniform_real_distribution<T>(-10, 10)(engine);
        const T slope = std::uniform_real_distribution<T>(-10, 10)(engine);

        return {
                .intercept = intercept,
                .slope = slope,
        };
}

template <typename T>
std::vector<numerical::Vector<2, T>> line_points(
        const linear_fit::Line<T>& line,
        const T x_min,
        const T x_max,
        const T noise_stddev,
        const std::size_t count,
        PCG& engine)
{
        std::uniform_real_distribution<T> x_d(x_min, x_max);
        std::normal_distribution<T> noise_d(0, noise_stddev);

        std::vector<numerical::Vector<2, T>> res;
        res.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
                const T x = x_d(engine);
                const T y = line.intercept + line.slope * x + noise_d(engine);
                res.push_back({x, y});
        }

        return res;
}

template <typename T>
std::vector<numerical::Vector<2, T>> add_outliers(
        std::vector<numerical::Vector<2, T>> points,
        const linear_fit::Line<T>& line,
        const T x_min,
        const T x_max,
        const T outlier_probability,
        const T outlier_min,
        const T outlier_max,
        PCG& engine)
{
        const std::size_t outlier_count = std::lround(points.size() * outlier_probability / (1 - outlier_probability));

        std::uniform_real_distribution<T> x_d(x_min, x_max);
        std::uniform_real_distribution<T> outlier_d(outlier_min, outlier_max);

        for (std::size_t i = 0; i < outlier_count; ++i)
        {
                const T x = x_d(engine);
                const T y = line.intercept + line.slope * x + outlier_d(engine);
                points.push_back({x, y});
        }

        std::ranges::shuffle(points, engine);

        return points;
}

template <typename T>
void test_qr(
        const std::vector<numerical::Vector<2, T>>& points,
        const linear_fit::Line<T>& etalon_line,
        const T stddev,
        const T max_error)
{
        const linear_fit::Line line = linear_fit::linear_fit_qr(points);

        const T rmse = compute_rmse(points, etalon_line, line);

        COMPARE_LESS(rmse, stddev * max_error);
}

template <typename T>
void test_theil_sen(
        const std::vector<numerical::Vector<2, T>>& points,
        const linear_fit::Line<T>& etalon_line,
        const T stddev,
        const T max_error)
{
        const linear_fit::Line line = linear_fit::linear_fit_theil_sen(points);

        const T rmse = compute_rmse(points, etalon_line, line);

        COMPARE_LESS(rmse, stddev * max_error);
}

template <typename T>
void test_medians(
        const std::vector<numerical::Vector<2, T>>& points,
        const linear_fit::Line<T>& etalon_line,
        const T stddev,
        const T max_error)
{
        const linear_fit::Line line = linear_fit::linear_fit_medians(points);

        const T rmse = compute_rmse(points, etalon_line, line);

        COMPARE_LESS(rmse, stddev * max_error);
}

template <typename T>
void test_linear_piecewise(PCG& engine)
{
        const int count = 1'000;
        const T noise_stddev = static_cast<T>(0.1);

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

        const std::vector<T> betas_s = linear_fit::linear_fit_piecewise(points, breakpoints);

        const T rmse = compute_rmse(points, breakpoints, betas, betas_s);

        COMPARE_LESS(rmse, 6.2 * expected_stddev(noise_stddev, count));
}

template <typename T>
void test_linear_piecewise_with_filter(PCG& engine)
{
        const T gate = 4;
        const T noise_stddev = static_cast<T>(0.1);
        const T outlier_p = static_cast<T>(0.4);
        const int count = std::lround(1'000 / (1 - outlier_p));

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
        std::bernoulli_distribution outlier(static_cast<double>(outlier_p));
        std::uniform_real_distribution<T> urd_outlier(-1, 10);

        std::vector<numerical::Vector<2, T>> points;
        points.reserve(count);

        for (int i = 0; i < count; ++i)
        {
                const T x = urd_x(engine);
                const T y = piecewise_function(x, betas, breakpoints)
                            + (outlier(engine) ? urd_outlier(engine) : nd_y(engine));
                points.push_back({x, y});
        }

        const std::vector<numerical::Vector<2, T>> filtered = linear_fit::filter_points(points, breakpoints, gate);
        const std::vector<T> betas_s = linear_fit::linear_fit_piecewise(filtered, breakpoints);

        const T rmse = compute_rmse(points, breakpoints, betas, betas_s);

        COMPARE_LESS(rmse, 12.1 * expected_stddev(noise_stddev, count, outlier_p));
}

template <typename T>
void test(PCG& engine)
{
        const linear_fit::Line<T> line = random_line<T>(engine);

        const std::size_t data_size = 1'000;
        const T x_min = -100;
        const T x_max = 100;

        const T noise_stddev = 0.1;
        const T stddev = noise_stddev / std::sqrt(data_size);

        const T gate = 4;
        const T outlier_probability = 0.25;
        const T outlier_min = -1;
        const T outlier_max = 10;

        const std::vector<numerical::Vector<2, T>> points =
                line_points(line, x_min, x_max, noise_stddev, data_size, engine);

        const std::vector<numerical::Vector<2, T>> outliers =
                add_outliers(points, line, x_min, x_max, outlier_probability, outlier_min, outlier_max, engine);

        const std::vector<numerical::Vector<2, T>> filtered = linear_fit::filter_points(outliers, gate);

        test_qr<T>(points, line, stddev, 5.0);
        test_qr<T>(outliers, line, stddev, 450.0);
        test_qr<T>(filtered, line, stddev, 5.3);

        test_theil_sen<T>(points, line, stddev, 5.0);
        test_theil_sen<T>(outliers, line, stddev, 17.4);
        test_theil_sen<T>(filtered, line, stddev, 5.8);

        test_medians<T>(points, line, stddev, 5.0);
        test_medians<T>(outliers, line, stddev, 17.4);
        test_medians<T>(filtered, line, stddev, 5.8);

        test_linear_piecewise<T>(engine);
        test_linear_piecewise_with_filter<T>(engine);
}

void test_median()
{
        LOG("Test linear fit");

        PCG engine;

        test<float>(engine);
        test<double>(engine);
        test<long double>(engine);

        LOG("Test linear fit passed");
}

TEST_SMALL("Linear Fit", test_median)
}
}
