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

#include "filters/filter.h"
#include "view/write.h"

#include <src/color/rgb8.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/filter/core/consistency.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ns::filter::core::test
{
namespace
{
template <typename T>
struct TimeUpdateInfo final
{
        T time;
        filters::UpdateInfo<T> info;
};

template <typename T>
struct TestResult final
{
        std::vector<TimeUpdateInfo<T>> result;
        Distribution<T> distribution;
        NormalizedSquared<T> nees;
};

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
std::vector<view::Point<T>> view_points(const std::vector<TimeUpdateInfo<T>>& result)
{
        std::vector<view::Point<T>> res;
        res.reserve(result.size());
        for (const TimeUpdateInfo<T>& r : result)
        {
                res.push_back({.time = r.time, .x = r.info.x, .stddev = r.info.stddev});
        }
        return res;
}

template <typename T>
std::vector<Measurements<T>> reset_v(const std::vector<Measurements<T>>& measurements)
{
        std::vector<Measurements<T>> res(measurements);
        for (Measurements<T>& m : res)
        {
                m.v.reset();
        }
        return res;
}

template <typename T>
TestResult<T> test_filter(filters::Filter<T>* const filter, const std::vector<Measurements<T>>& measurements)
{
        filter->reset();

        std::vector<TimeUpdateInfo<T>> result;
        Distribution<T> distribution;

        for (const Measurements<T>& m : measurements)
        {
                const auto update = filter->update(m);
                if (!update)
                {
                        continue;
                }
                result.push_back({.time = m.time, .info = *update});
                distribution.add(update->x - m.true_x, update->stddev);
        }

        return {.result = result, .distribution = distribution, .nees = filter->nees()};
}

template <typename T>
void test_impl(
        const std::string_view name,
        std::unique_ptr<filters::Filter<T>> filter,
        const std::vector<Measurements<T>>& measurements,
        const T precision_x,
        const T precision_xv,
        const T expected_stddev_x,
        const T expected_stddev_xv,
        const T stddev_count,
        const std::vector<unsigned>& expected_distribution,
        const std::array<T, 2>& min_max_nees_x,
        const std::array<T, 2>& min_max_nees_xv)
{
        const TestResult<T> result_x = test_filter(filter.get(), reset_v(measurements));

        {
                const auto& info = result_x.result.back().info;
                compare(info.stddev, expected_stddev_x, precision_x);
                compare(measurements.back().true_x, info.x, stddev_count * info.stddev);
        }

        if (const auto average = result_x.nees.average(); !(average > min_max_nees_x[0] && average < min_max_nees_x[1]))
        {
                error("NEES X; " + result_x.nees.check_string());
        }

        result_x.distribution.check(expected_distribution);

        //

        const TestResult<T> result_xv = test_filter(filter.get(), measurements);

        {
                const auto& info = result_xv.result.back().info;
                compare(info.stddev, expected_stddev_xv, precision_xv);
                compare(measurements.back().true_x, info.x, stddev_count * info.stddev);
        }

        if (const T average = result_xv.nees.average(); !(average > min_max_nees_xv[0] && average < min_max_nees_xv[1]))
        {
                error("NEES XV; " + result_xv.nees.check_string());
        }

        //

        view::write(
                name, measurements,
                {view::Filter<T>("Position", color::RGB8(128, 0, 0), view_points(result_x.result)),
                 view::Filter<T>("Position Speed", color::RGB8(0, 128, 0), view_points(result_xv.result))});
}

template <typename T>
void test_impl(const std::type_identity_t<T> precision_x, const std::type_identity_t<T> precision_xv)
{
        constexpr T DT = 1;
        constexpr T PROCESS_VELOCITY_MEAN = 1;
        constexpr T PROCESS_VELOCITY_VARIANCE = square(0.1);
        constexpr T MEASUREMENT_VARIANCE_X = square(3);
        constexpr T MEASUREMENT_VARIANCE_V = square(0.03);
        constexpr T INIT_X = 0;
        constexpr T INIT_V = 0;
        constexpr T INIT_V_VARIANCE = 2;

        constexpr std::size_t COUNT = 1000;

        const std::vector<Measurements<T>> measurements = simulate<T>(
                COUNT, INIT_X, DT, PROCESS_VELOCITY_MEAN, PROCESS_VELOCITY_VARIANCE, MEASUREMENT_VARIANCE_X,
                MEASUREMENT_VARIANCE_V);

        const std::vector<unsigned> distribution = {580, 230, 60, 16, 7, 3, 0, 0, 0, 0};
        const std::array<T, 2> min_max_nees_x{0.4, 1.0};
        const std::array<T, 2> min_max_nees_xv{0.15, 2.95};

        test_impl<T>(
                "EKF", filters::create_ekf<T>(INIT_V, INIT_V_VARIANCE, PROCESS_VELOCITY_VARIANCE), measurements,
                precision_x, precision_xv, 1.4306576889002234962L, 0.298852051985480352583L, 5, distribution,
                min_max_nees_x, min_max_nees_xv);

        test_impl<T>(
                "H_INFINITY", filters::create_h_infinity<T>(INIT_V, INIT_V_VARIANCE, PROCESS_VELOCITY_VARIANCE),
                measurements, precision_x, precision_xv, 1.43098764352003224212L, 0.298852351050054556604L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);

        test_impl<T>(
                "UKF", filters::create_ukf<T>(INIT_V, INIT_V_VARIANCE, PROCESS_VELOCITY_VARIANCE), measurements,
                precision_x, precision_xv, 1.43670888967218343853L, 0.304462860888633687857L, 5, distribution,
                min_max_nees_x, min_max_nees_xv);
}

void test()
{
        LOG("Test Filter 1D");
        test_impl<float>(1e-3, 5e-3);
        test_impl<double>(1e-12, 5e-12);
        test_impl<long double>(1e-15, 3e-15);
        LOG("Test Filter 1D passed");
}

TEST_SMALL("Filter 1D", test)
}
}
