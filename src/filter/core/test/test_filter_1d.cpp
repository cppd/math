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
struct ProcessData final
{
        T true_x;
        T measurement_x;
        T measurement_v;
};

template <typename T>
struct ResultData final
{
        T x;
        T standard_deviation;
};

template <typename T, typename Engine>
std::vector<ProcessData<T>> simulate(
        const std::size_t count,
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

        T x = 0;
        T v = nd_process_v(engine);
        std::vector<ProcessData<T>> res;
        res.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
                x += dt * v;
                v = nd_process_v(engine);
                res.push_back(
                        {.true_x = x,
                         .measurement_x = x + nd_measurement_x(engine),
                         .measurement_v = v + nd_measurement_v(engine)});
        }
        return res;
}

template <typename T>
std::string make_string(const ProcessData<T>& process, const ResultData<T>& result, const ResultData<T>& result_xv)
{
        std::string res;
        res += '(' + to_string(process.true_x);
        res += ", " + to_string(process.measurement_x);
        res += ", " + to_string(result.x);
        res += ", " + to_string(result.standard_deviation);
        res += ", " + to_string(result_xv.x);
        res += ", " + to_string(result_xv.standard_deviation);
        res += ')';
        return res;
}

template <typename T>
void write_to_file(
        const std::string& file_name,
        const std::vector<ProcessData<T>>& process,
        const std::vector<ResultData<T>>& result,
        const std::vector<ResultData<T>>& result_v)
{
        ASSERT(process.size() == result.size());
        ASSERT(process.size() == result_v.size());

        std::ofstream file(utility::test_file_path(file_name));
        for (std::size_t i = 0; i < process.size(); ++i)
        {
                file << make_string(process[i], result[i], result_v[i]) << '\n';
        }
}

template <typename Filter>
std::vector<ResultData<typename Filter::Type>> test_filter_x(
        Filter* const filter,
        const std::vector<ProcessData<typename Filter::Type>>& process_data,
        const typename Filter::Type dt,
        const typename Filter::Type process_velocity_variance,
        const typename Filter::Type measurement_variance_x,
        const typename Filter::Type precision,
        const typename Filter::Type expected_deviation,
        const typename Filter::Type deviation_count,
        const std::vector<unsigned>& expected_distribution)
{
        using T = Filter::Type;

        Distribution<T> distribution;

        NormalizedSquared<T> nees;

        std::vector<ResultData<T>> result_data;
        result_data.reserve(process_data.size());
        for (const ProcessData<T>& process : process_data)
        {
                filter->predict(dt, process_velocity_variance);
                filter->update_position(process.measurement_x, measurement_variance_x);

                const T x = filter->position();
                const T variance = filter->position_p();
                const T stddev = std::sqrt(variance);

                result_data.push_back({.x = x, .standard_deviation = stddev});
                distribution.add(x - process.true_x, stddev);

                nees.add_1(process.true_x - x, variance);
        }

        compare(result_data.back().standard_deviation, expected_deviation, precision);
        compare(process_data.back().true_x, result_data.back().x,
                deviation_count * result_data.back().standard_deviation);

        const T nees_average = nees.average();
        if (!(nees_average > T{0.45} && nees_average < T{1.25}))
        {
                error("NEES; " + nees.check_string());
        }

        distribution.check(expected_distribution);

        return result_data;
}

template <typename Filter>
std::vector<ResultData<typename Filter::Type>> test_filter_xv(
        Filter* const filter,
        const std::vector<ProcessData<typename Filter::Type>>& process_data,
        const typename Filter::Type dt,
        const typename Filter::Type process_velocity_variance,
        const typename Filter::Type measurement_variance_x,
        const typename Filter::Type measurement_variance_v)
{
        using T = Filter::Type;

        std::vector<ResultData<T>> result_data;
        result_data.reserve(process_data.size());
        for (const ProcessData<T>& process : process_data)
        {
                filter->predict(dt, process_velocity_variance);
                filter->update_position_speed(
                        process.measurement_x, measurement_variance_x, process.measurement_v, measurement_variance_v);

                const T x = filter->position();
                const T variance = filter->position_p();
                const T stddev = std::sqrt(variance);

                result_data.push_back({.x = x, .standard_deviation = stddev});
        }

        return result_data;
}

template <typename Filter>
void test_impl(
        std::unique_ptr<Filter> filter,
        const typename Filter::Type precision,
        const typename Filter::Type expected_deviation,
        const typename Filter::Type deviation_count,
        const std::vector<unsigned>& expected_distribution)
{
        using T = Filter::Type;

        constexpr T DT = 1;
        constexpr T PROCESS_VELOCITY_MEAN = 1;
        constexpr T PROCESS_VELOCITY_VARIANCE = square(0.1);
        constexpr T MEASUREMENT_VARIANCE_X = square(3);
        constexpr T MEASUREMENT_VARIANCE_V = square(0.03);

        constexpr numerical::Vector<2, T> X(10, 5);
        constexpr numerical::Matrix<2, 2, T> P{
                {square(20),         0},
                {         0, square(7)}
        };

        constexpr std::size_t COUNT = 1000;

        const std::vector<ProcessData<T>> process_data = simulate<T>(
                COUNT, DT, PROCESS_VELOCITY_MEAN, PROCESS_VELOCITY_VARIANCE, MEASUREMENT_VARIANCE_X,
                MEASUREMENT_VARIANCE_V, PCG());

        filter->reset(X, P);
        const std::vector<ResultData<T>> result_data_x = test_filter_x(
                filter.get(), process_data, DT, PROCESS_VELOCITY_VARIANCE, MEASUREMENT_VARIANCE_X, precision,
                expected_deviation, deviation_count, expected_distribution);

        filter->reset(X, P);
        const std::vector<ResultData<T>> result_data_xv = test_filter_xv(
                filter.get(), process_data, DT, PROCESS_VELOCITY_VARIANCE, MEASUREMENT_VARIANCE_X,
                MEASUREMENT_VARIANCE_V);

        write_to_file(
                "filter_" + to_lower(filter->name()) + "_1d_" + utility::replace_space(type_name<T>()) + ".txt",
                process_data, result_data_x, result_data_xv);
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
