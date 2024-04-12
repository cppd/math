/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "distribution.h"
#include "measurements.h"
#include "simulator.h"

#include "filters/ekf.h"
#include "filters/ukf.h"
#include "view/write.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/filter/core/consistency.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

namespace ns::filter::core::test
{
namespace
{
template <typename T>
void compare(const T a, const T b, const T precision)
{
        static_assert(std::is_floating_point_v<T>);

        if (a == b)
        {
                return;
        }

        const T abs = std::abs(a - b);
        if (!(abs < precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b) + "; absolute " + to_string(abs)
                      + "; required precision " + to_string(precision));
        }
}

template <typename T>
struct Result final
{
        T time;
        T x;
        T stddev;
};

template <typename T>
std::vector<view::Point<T>> view_points(const std::vector<Result<T>>& result)
{
        std::vector<view::Point<T>> res;
        res.reserve(result.size());
        for (const Result<T>& r : result)
        {
                res.push_back({.time = r.time, .x = r.x, .stddev = r.stddev});
        }
        return res;
}

template <typename T>
struct TestResult final
{
        std::vector<Result<T>> result;
        Distribution<T> distribution;
        NormalizedSquared<T> nees;
};

template <typename Filter>
TestResult<typename Filter::Type> test_filter_x(
        Filter* const filter,
        const std::vector<Measurements<typename Filter::Type>>& measurements,
        const typename Filter::Type process_variance)
{
        using T = Filter::Type;

        Distribution<T> distribution;
        NormalizedSquared<T> nees;

        auto iter = measurements.cbegin();
        ASSERT(iter != measurements.end());

        {
                const numerical::Vector<2, T> x(iter->x, iter->v);
                const numerical::Matrix<2, 2, T> p{
                        {iter->x_variance,                0},
                        {               0, iter->v_variance}
                };
                filter->reset(x, p);
        }

        T last_time = iter->time;

        std::vector<Result<T>> res;
        res.reserve(measurements.size());

        res.push_back({.time = iter->time, .x = filter->position(), .stddev = std::sqrt(filter->position_p())});

        ASSERT(iter != measurements.end());
        for (++iter; iter != measurements.end(); ++iter)
        {
                const Measurements<T>& m = *iter;

                const T dt = m.time - last_time;
                ASSERT(dt >= 0);
                last_time = m.time;

                filter->predict(dt, process_variance);
                filter->update_position(m.x, m.x_variance);

                const T x = filter->position();
                const T variance = filter->position_p();
                const T stddev = std::sqrt(variance);

                res.push_back({.time = m.time, .x = x, .stddev = stddev});

                distribution.add(x - m.true_x, stddev);
                nees.add_1(m.true_x - x, variance);
        }

        return {.result = res, .distribution = distribution, .nees = nees};
}

template <typename Filter>
TestResult<typename Filter::Type> test_filter_xv(
        Filter* const filter,
        const std::vector<Measurements<typename Filter::Type>>& measurements,
        const typename Filter::Type process_variance)
{
        using T = Filter::Type;

        Distribution<T> distribution;
        NormalizedSquared<T> nees;

        auto iter = measurements.cbegin();
        ASSERT(iter != measurements.end());

        {
                const numerical::Vector<2, T> x(iter->x, iter->v);
                const numerical::Matrix<2, 2, T> p{
                        {iter->x_variance,                0},
                        {               0, iter->v_variance}
                };
                filter->reset(x, p);
        }

        T last_time = iter->time;

        std::vector<Result<T>> res;
        res.reserve(measurements.size());

        res.push_back({.time = iter->time, .x = filter->position(), .stddev = std::sqrt(filter->position_p())});

        for (++iter; iter != measurements.end(); ++iter)
        {
                const Measurements<T>& m = *iter;

                const T dt = m.time - last_time;
                ASSERT(dt >= 0);
                last_time = m.time;

                filter->predict(dt, process_variance);
                filter->update_position_speed(m.x, m.x_variance, m.v, m.v_variance);

                const T x = filter->position();
                const T variance = filter->position_p();
                const T stddev = std::sqrt(variance);

                res.push_back({.time = m.time, .x = x, .stddev = stddev});

                distribution.add(x - m.true_x, stddev);
                nees.add(
                        numerical::Vector<2, T>(m.true_x, m.true_v) - filter->position_speed(),
                        filter->position_speed_p());
        }

        return {.result = res, .distribution = distribution, .nees = nees};
}

template <typename Filter>
void test_impl(
        std::unique_ptr<Filter> filter,
        const typename Filter::Type precision_x,
        const typename Filter::Type precision_xv,
        const typename Filter::Type expected_stddev_x,
        const typename Filter::Type expected_stddev_xv,
        const typename Filter::Type stddev_count,
        const std::vector<unsigned>& expected_distribution)
{
        using T = Filter::Type;

        constexpr T DT = 1;
        constexpr T PROCESS_VELOCITY_MEAN = 1;
        constexpr T PROCESS_VELOCITY_VARIANCE = square(0.1);
        constexpr T MEASUREMENT_VARIANCE_X = square(3);
        constexpr T MEASUREMENT_VARIANCE_V = square(0.03);
        constexpr T INIT_X = 0;

        constexpr std::size_t COUNT = 1000;

        const std::vector<Measurements<T>> measurements = simulate<T>(
                COUNT, INIT_X, DT, PROCESS_VELOCITY_MEAN, PROCESS_VELOCITY_VARIANCE, MEASUREMENT_VARIANCE_X,
                MEASUREMENT_VARIANCE_V);

        //

        const TestResult<T> result_x = test_filter_x(filter.get(), measurements, PROCESS_VELOCITY_VARIANCE);

        compare(result_x.result.back().stddev, expected_stddev_x, precision_x);
        compare(measurements.back().true_x, result_x.result.back().x, stddev_count * result_x.result.back().stddev);

        if (const auto average = result_x.nees.average(); !(average > T{0.45} && average < T{1.25}))
        {
                error("NEES X; " + result_x.nees.check_string());
        }

        result_x.distribution.check(expected_distribution);

        //

        const TestResult<T> result_xv = test_filter_xv(filter.get(), measurements, PROCESS_VELOCITY_VARIANCE);

        compare(result_xv.result.back().stddev, expected_stddev_xv, precision_xv);
        compare(measurements.back().true_x, result_xv.result.back().x, stddev_count * result_xv.result.back().stddev);

        if (const T average = result_xv.nees.average(); !(average > T{0.15} && average < T{2.95}))
        {
                error("NEES XV; " + result_xv.nees.check_string());
        }

        //

        view::write(filter->name(), measurements, view_points(result_x.result), view_points(result_xv.result));
}

template <typename T>
void test_impl(const std::type_identity_t<T> precision_x, const std::type_identity_t<T> precision_xv)
{
        const std::vector<unsigned> distribution = {580, 230, 60, 16, 7, 3, 0, 0, 0, 0};

        test_impl(
                filters::create_filter_ekf<T, false>(), precision_x, precision_xv, 1.4306576889002234962L,
                0.298852051985480352583L, 5, distribution);

        test_impl(
                filters::create_filter_ekf<T, true>(), precision_x, precision_xv, 1.43098764352003224212L,
                0.298852351050054556604L, 5, distribution);

        test_impl(
                filters::create_filter_ukf<T>(), precision_x, precision_xv, 1.43670888967218343853L,
                0.304462860888633687857L, 5, distribution);
}

void test()
{
        LOG("Test Filter 1D");
        test_impl<float>(1e-3, 5e-3);
        test_impl<double>(1e-12, 5e-12);
        test_impl<long double>(1e-15, 2e-15);
        LOG("Test Filter 1D passed");
}

TEST_SMALL("Filter 1D", test)
}
}
