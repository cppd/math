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
#include "ekf.h"
#include "ukf.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/string/str.h>
#include <src/com/type/name.h>
#include <src/filter/core/consistency.h>
#include <src/filter/utility/files.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <fstream>
#include <memory>
#include <random>
#include <string>
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
struct Measurements final
{
        T true_x;
        T x;
        T v;
};

template <typename T>
struct Result final
{
        T x;
        T stddev;
};

template <typename T, typename Engine>
std::vector<Measurements<T>> simulate(
        const std::size_t count,
        const T init_x,
        const T dt,
        const T process_velocity_mean,
        const T process_velocity_variance,
        const T measurement_variance_x,
        const T measurement_variance_v,
        Engine engine)
{
        std::normal_distribution<T> nd_process_v(process_velocity_mean, std::sqrt(process_velocity_variance));
        std::normal_distribution<T> nd_measurement_x(0, std::sqrt(measurement_variance_x));
        std::normal_distribution<T> nd_measurement_v(0, std::sqrt(measurement_variance_v));

        std::vector<Measurements<T>> res;
        res.reserve(count);

        const auto push = [&](const T x, const T v)
        {
                const T m_x = x + nd_measurement_x(engine);
                const T m_v = v + nd_measurement_v(engine);
                res.push_back({.true_x = x, .x = m_x, .v = m_v});
        };

        T x = init_x;
        T v = nd_process_v(engine);
        push(x, v);
        for (std::size_t i = 1; i < count; ++i)
        {
                const T v_next = nd_process_v(engine);
                const T v_average = (v + v_next) / 2;
                x += dt * v_average;
                v = v_next;
                push(x, v);
        }

        return res;
}

template <typename T>
std::string make_string(const Measurements<T>& process, const Result<T>& result_x, const Result<T>& result_xv)
{
        std::string res;
        res += '(' + to_string(process.true_x);
        res += ", " + to_string(process.x);
        res += ", " + to_string(result_x.x);
        res += ", " + to_string(result_x.stddev);
        res += ", " + to_string(result_xv.x);
        res += ", " + to_string(result_xv.stddev);
        res += ')';
        return res;
}

template <typename T>
void write_to_file(
        const std::string& file_name,
        const std::vector<Measurements<T>>& process,
        const std::vector<Result<T>>& result_x,
        const std::vector<Result<T>>& result_xv)
{
        ASSERT(process.size() == result_x.size());
        ASSERT(process.size() == result_xv.size());

        std::ofstream file(utility::test_file_path(file_name));
        for (std::size_t i = 0; i < process.size(); ++i)
        {
                file << make_string(process[i], result_x[i], result_xv[i]) << '\n';
        }
}

template <typename Filter>
std::vector<Result<typename Filter::Type>> test_filter_x(
        Filter* const filter,
        const std::vector<Measurements<typename Filter::Type>>& process_data,
        const typename Filter::Type dt,
        const typename Filter::Type process_velocity_variance,
        const typename Filter::Type measurement_variance_x,
        const typename Filter::Type precision,
        const typename Filter::Type expected_stddev,
        const typename Filter::Type stddev_count,
        const std::vector<unsigned>& expected_distribution)
{
        using T = Filter::Type;

        Distribution<T> distribution;

        NormalizedSquared<T> nees;

        std::vector<Result<T>> res;
        res.reserve(process_data.size());
        for (const Measurements<T>& process : process_data)
        {
                filter->predict(dt, process_velocity_variance);
                filter->update_position(process.x, measurement_variance_x);

                const T x = filter->position();
                const T variance = filter->position_p();
                const T stddev = std::sqrt(variance);

                res.push_back({.x = x, .stddev = stddev});
                distribution.add(x - process.true_x, stddev);

                nees.add_1(process.true_x - x, variance);
        }

        compare(res.back().stddev, expected_stddev, precision);
        compare(process_data.back().true_x, res.back().x, stddev_count * res.back().stddev);

        const T nees_average = nees.average();
        if (!(nees_average > T{0.45} && nees_average < T{1.25}))
        {
                error("NEES; " + nees.check_string());
        }

        distribution.check(expected_distribution);

        return res;
}

template <typename Filter>
std::vector<Result<typename Filter::Type>> test_filter_xv(
        Filter* const filter,
        const std::vector<Measurements<typename Filter::Type>>& process_data,
        const typename Filter::Type dt,
        const typename Filter::Type process_velocity_variance,
        const typename Filter::Type measurement_variance_x,
        const typename Filter::Type measurement_variance_v)
{
        using T = Filter::Type;

        std::vector<Result<T>> res;
        res.reserve(process_data.size());
        for (const Measurements<T>& process : process_data)
        {
                filter->predict(dt, process_velocity_variance);
                filter->update_position_speed(process.x, measurement_variance_x, process.v, measurement_variance_v);

                const T x = filter->position();
                const T variance = filter->position_p();
                const T stddev = std::sqrt(variance);

                res.push_back({.x = x, .stddev = stddev});
        }

        return res;
}

template <typename Filter>
void test_impl(
        std::unique_ptr<Filter> filter,
        const typename Filter::Type precision,
        const typename Filter::Type expected_stddev,
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

        constexpr numerical::Vector<2, T> X(INIT_X + 10, PROCESS_VELOCITY_MEAN + 5);
        constexpr numerical::Matrix<2, 2, T> P{
                {square(15),           0},
                {         0, square(7.5)}
        };

        constexpr std::size_t COUNT = 1000;

        const std::vector<Measurements<T>> process_data = simulate<T>(
                COUNT, INIT_X, DT, PROCESS_VELOCITY_MEAN, PROCESS_VELOCITY_VARIANCE, MEASUREMENT_VARIANCE_X,
                MEASUREMENT_VARIANCE_V, PCG());

        filter->reset(X, P);
        const std::vector<Result<T>> result_x = test_filter_x(
                filter.get(), process_data, DT, PROCESS_VELOCITY_VARIANCE, MEASUREMENT_VARIANCE_X, precision,
                expected_stddev, stddev_count, expected_distribution);

        filter->reset(X, P);
        const std::vector<Result<T>> result_xv = test_filter_xv(
                filter.get(), process_data, DT, PROCESS_VELOCITY_VARIANCE, MEASUREMENT_VARIANCE_X,
                MEASUREMENT_VARIANCE_V);

        write_to_file(
                "filter_" + to_lower(filter->name()) + "_1d_" + utility::replace_space(type_name<T>()) + ".txt",
                process_data, result_x, result_xv);
}

template <typename T>
void test_impl(const std::type_identity_t<T> precision)
{
        const std::vector<unsigned> distribution = {580, 230, 60, 16, 7, 3, 0, 0, 0, 0};
        test_impl<TestEkf<T, false>>(create_test_ekf<T, false>(), precision, 1.4306576889002234962L, 5, distribution);
        test_impl<TestEkf<T, true>>(create_test_ekf<T, true>(), precision, 1.43098764352003224212L, 5, distribution);
        test_impl<TestUkf<T>>(create_test_ukf<T>(), precision, 1.43670888967218343853L, 5, distribution);
}

void test()
{
        LOG("Test Filter 1D");
        test_impl<float>(1e-3);
        test_impl<double>(1e-12);
        test_impl<long double>(1e-15);
        LOG("Test Filter 1D passed");
}

TEST_SMALL("Filter 1D", test)
}
}
