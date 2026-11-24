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
#include <src/statistics/linear_fit/linear_filter.h>
#include <src/statistics/linear_fit/linear_fit.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace ns::statistics::test
{
namespace
{
template <typename T>
class PiecewiseFunction final
{
        std::vector<T> breakpoints_;
        std::vector<T> betas_;

public:
        PiecewiseFunction(const std::vector<T>& breakpoints, PCG& engine)
                : breakpoints_(breakpoints)
        {
                std::uniform_real_distribution<T> beta_d(-10, 10);

                const std::size_t count = breakpoints_.size() + 1;
                betas_.reserve(count);
                for (std::size_t i = 0; i < count; ++i)
                {
                        betas_.push_back(beta_d(engine));
                }

                ASSERT(betas_.size() == breakpoints_.size() + 1);
        }

        PiecewiseFunction(const std::vector<T>& breakpoints, const std::vector<T>& betas)
                : breakpoints_(std::move(breakpoints)),
                  betas_(std::move(betas))
        {
                ASSERT(betas_.size() == breakpoints_.size() + 1);
        }

        [[nodiscard]] T operator()(const T x) const
        {
                T res = betas_[0];
                for (std::size_t b = 0; b < breakpoints_.size(); ++b)
                {
                        if (x > breakpoints_[b])
                        {
                                res += betas_[b + 1] * (x - breakpoints_[b]);
                        }
                }
                return res;
        }
};

template <typename T>
class LineFunction final
{
        const linear_fit::Line<T> line_;

public:
        explicit LineFunction(const linear_fit::Line<T>& line)
                : line_(line)
        {
        }

        explicit LineFunction(PCG& engine)
                : line_{
                          .intercept = std::uniform_real_distribution<T>(-10, 10)(engine),
                          .slope = std::uniform_real_distribution<T>(-10, 10)(engine),
                  }
        {
        }

        [[nodiscard]] T operator()(const T x) const
        {
                return line_.intercept + line_.slope * x;
        }
};

template <typename T>
struct LineData final
{
        LineFunction<T> etalon_f;
        T stddev;
        std::vector<numerical::Vector<2, T>> points;
        std::vector<numerical::Vector<2, T>> outliers;
        std::vector<numerical::Vector<2, T>> filtered;
};

template <typename T>
LineData<T> create_line_data(PCG& engine)
{
        const LineFunction<T> etalon_f(engine);

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
                line_points(etalon_f, x_min, x_max, noise_stddev, data_size, engine);

        const std::vector<numerical::Vector<2, T>> outliers =
                add_outliers(points, etalon_f, x_min, x_max, outlier_probability, outlier_min, outlier_max, engine);

        const std::vector<numerical::Vector<2, T>> filtered = linear_fit::filter_points(outliers, gate);

        return {
                .etalon_f = etalon_f,
                .stddev = stddev,
                .points = points,
                .outliers = outliers,
                .filtered = filtered,
        };
}

template <typename T>
struct PiecewiseData final
{
        PiecewiseFunction<T> etalon_f;
        T stddev;
        std::vector<T> breakpoints;
        std::vector<numerical::Vector<2, T>> points;
        std::vector<numerical::Vector<2, T>> outliers;
        std::vector<numerical::Vector<2, T>> filtered;
};

template <typename T>
PiecewiseData<T> create_piecewise_data(PCG& engine)
{
        const std::size_t data_size = 1'000;

        const T x_min = -100;
        const T x_max = 100;

        const T noise_stddev = 0.1;
        const T stddev = noise_stddev / std::sqrt(data_size);

        const std::vector<T> breakpoints{
                -100,
                std::uniform_real_distribution<T>(-51, -49)(engine),
                std::uniform_real_distribution<T>(-1, 1)(engine),
                std::uniform_real_distribution<T>(49, 51)(engine),
        };

        const T gate = 4;
        const T outlier_probability = 0.25;
        const T outlier_min = -1;
        const T outlier_max = 10;

        const PiecewiseFunction<T> etalon_f(breakpoints, engine);

        const std::vector<numerical::Vector<2, T>> points =
                line_points(etalon_f, x_min, x_max, noise_stddev, data_size, engine);

        const std::vector<numerical::Vector<2, T>> outliers =
                add_outliers(points, etalon_f, x_min, x_max, outlier_probability, outlier_min, outlier_max, engine);

        const std::vector<numerical::Vector<2, T>> filtered = linear_fit::filter_points(outliers, breakpoints, gate);

        return {
                .etalon_f = etalon_f,
                .stddev = stddev,
                .breakpoints = breakpoints,
                .points = points,
                .outliers = outliers,
                .filtered = filtered,
        };
}

template <typename T, typename F>
T compute_rmse(const std::vector<numerical::Vector<2, T>>& points, const F& f1, const F& f2)
{
        T sum = 0;
        for (const numerical::Vector<2, T>& p : points)
        {
                const T x = p[0];
                const T d = f1(x) - f2(x);
                sum += d * d;
        }
        return std::sqrt(sum / points.size());
}

template <typename T, typename F>
std::vector<numerical::Vector<2, T>> line_points(
        const F& f,
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
                const T y = f(x) + noise_d(engine);
                res.push_back({x, y});
        }

        return res;
}

template <typename T, typename F>
std::vector<numerical::Vector<2, T>> add_outliers(
        std::vector<numerical::Vector<2, T>> points,
        const F& f,
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
                const T y = f(x) + outlier_d(engine);
                points.push_back({x, y});
        }

        std::ranges::shuffle(points, engine);

        return points;
}

template <typename T, typename F>
void check_function(
        const std::string_view name,
        const std::vector<numerical::Vector<2, T>>& points,
        const F& f,
        const F& etalon_f,
        const T stddev,
        const T max_error)
{
        const T rmse = compute_rmse(points, etalon_f, f);
        const T max_rmse = stddev * max_error;

        if (!(rmse <= max_rmse))
        {
                error(std::string(name) + ": " + to_string(rmse) + " is not less than " + to_string(max_rmse));
        }
}

template <typename T>
void linear_fit_qr(
        const std::string_view name,
        const std::vector<numerical::Vector<2, T>>& points,
        const LineFunction<T>& etalon_f,
        const T stddev,
        const T max_error)
{
        const linear_fit::Line line = linear_fit::linear_fit_qr(points);
        const LineFunction f(line);
        check_function(name, points, f, etalon_f, stddev, max_error);
}

template <typename T>
void linear_fit_theil_sen(
        const std::string_view name,
        const std::vector<numerical::Vector<2, T>>& points,
        const LineFunction<T>& etalon_f,
        const T stddev,
        const T max_error)
{
        const linear_fit::Line line = linear_fit::linear_fit_theil_sen(points);
        const LineFunction f(line);
        check_function(name, points, f, etalon_f, stddev, max_error);
}

template <typename T>
void linear_fit_medians(
        const std::string_view name,
        const std::vector<numerical::Vector<2, T>>& points,
        const LineFunction<T>& etalon_f,
        const T stddev,
        const T max_error)
{
        const linear_fit::Line line = linear_fit::linear_fit_medians(points);
        const LineFunction f(line);
        check_function(name, points, f, etalon_f, stddev, max_error);
}

template <typename T>
void linear_fit_qr_piecewise(
        const std::string_view name,
        const std::vector<numerical::Vector<2, T>>& points,
        const PiecewiseFunction<T>& etalon_f,
        const std::vector<T>& breakpoints,
        const T stddev,
        const T max_error)
{
        const std::vector<T> betas_s = linear_fit::linear_fit_qr_piecewise(points, breakpoints);
        const PiecewiseFunction<T> f(breakpoints, betas_s);
        check_function(name, points, f, etalon_f, stddev, max_error);
}

template <typename T>
void test_piecewise(PCG& engine)
{
        const PiecewiseData<T> pd = create_piecewise_data<T>(engine);

        linear_fit_qr_piecewise<T>("piecewise points", pd.points, pd.etalon_f, pd.breakpoints, pd.stddev, 5.8);
        linear_fit_qr_piecewise<T>("piecewise outliers", pd.outliers, pd.etalon_f, pd.breakpoints, pd.stddev, 450.0);
        linear_fit_qr_piecewise<T>("piecewise filtered", pd.filtered, pd.etalon_f, pd.breakpoints, pd.stddev, 6.3);
}

template <typename T>
void test_line(PCG& engine)
{
        const LineData<T> ld = create_line_data<T>(engine);

        linear_fit_qr<T>("qr points", ld.points, ld.etalon_f, ld.stddev, 5.0);
        linear_fit_qr<T>("qr outliers", ld.outliers, ld.etalon_f, ld.stddev, 450.0);
        linear_fit_qr<T>("qr filtered", ld.filtered, ld.etalon_f, ld.stddev, 5.3);

        linear_fit_theil_sen<T>("theil sen points", ld.points, ld.etalon_f, ld.stddev, 5.0);
        linear_fit_theil_sen<T>("theil sen outliers", ld.outliers, ld.etalon_f, ld.stddev, 17.4);
        linear_fit_theil_sen<T>("theil sen filtered", ld.filtered, ld.etalon_f, ld.stddev, 5.8);

        linear_fit_medians<T>("medians points", ld.points, ld.etalon_f, ld.stddev, 5.0);
        linear_fit_medians<T>("medians outliers", ld.outliers, ld.etalon_f, ld.stddev, 17.4);
        linear_fit_medians<T>("medians filtered", ld.filtered, ld.etalon_f, ld.stddev, 5.8);
}

template <typename T>
void test(PCG& engine)
{
        test_line<T>(engine);
        test_piecewise<T>(engine);
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
