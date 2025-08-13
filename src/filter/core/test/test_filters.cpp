/*
Copyright (C) 2017-2025 Topological Manifold

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
#include "smooth.h"
#include "time_update_info.h"

#include "filters/filter.h"
#include "filters/filter_info.h"
#include "filters/filters.h"
#include "filters/noise_model.h"
#include "simulator/speed.h"
#include "view/point.h"
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
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ns::filter::core::test
{
namespace
{
constexpr unsigned SMOOTH_LAG = 5;

template <typename T>
constexpr T DATA_CONNECT_INTERVAL = 10;

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
std::vector<Measurements<T>> reset_v(const std::vector<Measurements<T>>& measurements)
{
        std::vector<Measurements<T>> res(measurements);
        for (Measurements<T>& m : res)
        {
                m.speed.reset();
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
                distribution.add(update->position - m.true_position, update->position_stddev);
        }

        return {.result = result, .distribution = distribution, .nees = filter->nees()};
}

template <typename T>
std::vector<view::Point<T>> view_points(const std::vector<TimeUpdateInfo<T>>& info)
{
        std::vector<view::Point<T>> res;
        res.reserve(info.size());
        for (const TimeUpdateInfo<T>& r : info)
        {
                res.push_back(
                        {.time = r.time,
                         .position = r.info.position,
                         .position_stddev = r.info.position_stddev,
                         .speed = r.info.speed,
                         .speed_stddev = r.info.speed_stddev});
        }
        return res;
}

template <typename T>
void test_impl(
        const std::string_view name,
        const std::string_view annotation,
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
                compare(info.position_stddev, expected_stddev_x, precision_x);
                compare(measurements.back().true_position, info.position, stddev_count * info.position_stddev);
        }

        if (const auto average = result_x.nees.average(); !(average > min_max_nees_x[0] && average < min_max_nees_x[1]))
        {
                error("NEES X; " + result_x.nees.check_string());
        }

        result_x.distribution.check(expected_distribution);

        const std::vector<view::Point<T>> smooth_view_points_x_all = smooth_all(result_x.result);
        const std::vector<view::Point<T>> smooth_view_points_x_lag = smooth_lag(result_x.result, SMOOTH_LAG);

        //

        const TestResult<T> result_xv = test_filter(filter.get(), measurements);

        {
                const auto& info = result_xv.result.back().info;
                compare(info.position_stddev, expected_stddev_xv, precision_xv);
                compare(measurements.back().true_position, info.position, stddev_count * info.position_stddev);
        }

        if (const T average = result_xv.nees.average(); !(average > min_max_nees_xv[0] && average < min_max_nees_xv[1]))
        {
                error("NEES XV; " + result_xv.nees.check_string());
        }

        const std::vector<view::Point<T>> smooth_view_points_xv_all = smooth_all(result_xv.result);
        const std::vector<view::Point<T>> smooth_view_points_xv_lag = smooth_lag(result_xv.result, SMOOTH_LAG);

        //

        view::write(
                name, annotation, measurements, DATA_CONNECT_INTERVAL<T>,
                {
                        view::Filter<T>("Position", color::RGB8(128, 0, 0), view_points(result_x.result)),
                        view::Filter<T>("Speed", color::RGB8(0, 128, 0), view_points(result_xv.result)),
                        view::Filter<T>("Smooth All Position", color::RGB8(0, 170, 0), smooth_view_points_x_all),
                        view::Filter<T>("Smooth All Speed", color::RGB8(0, 200, 0), smooth_view_points_xv_all),
                        view::Filter<T>("Smooth Lag Position", color::RGB8(200, 0, 255), smooth_view_points_x_lag),
                        view::Filter<T>("Smooth Lag Speed", color::RGB8(250, 0, 255), smooth_view_points_xv_lag),
                });
}

template <typename T>
std::string make_annotation(
        const T simulation_dt,
        const T simulation_velocity_variance,
        const T simulation_measurement_variance_x,
        const T simulation_measurement_variance_v)
{
        constexpr std::string_view SIGMA = "&#x03c3;";

        std::ostringstream oss;

        oss << "<b>update</b>";
        oss << "<br>";
        oss << "position: " << 1 / simulation_dt << " Hz";

        oss << "<br>";
        oss << "<br>";
        oss << "<b>" << SIGMA << "</b>";
        oss << "<br>";
        oss << "process speed: " << std::sqrt(simulation_velocity_variance) << " m/s";
        oss << "<br>";
        oss << "position: " << std::sqrt(simulation_measurement_variance_x) << " m";
        oss << "<br>";
        oss << "speed: " << std::sqrt(simulation_measurement_variance_v) << " m/s";

        return oss.str();
}

template <typename T>
void test_impl(const std::type_identity_t<T> precision_x, const std::type_identity_t<T> precision_xv)
{
        constexpr T SIMULATION_LENGTH = 1000;

        constexpr T SIMULATION_DT = 1;
        constexpr T SIMULATION_VELOCITY_MEAN = 1;
        constexpr T SIMULATION_VELOCITY_VARIANCE = square(0.1);
        constexpr T SIMULATION_MEASUREMENT_VARIANCE_X = square(3);
        constexpr T SIMULATION_MEASUREMENT_VARIANCE_V = square(0.03);
        constexpr T SIMULATION_INIT_X = 0;

        constexpr T FILTER_INIT_V = 0;
        constexpr T FILTER_INIT_V_VARIANCE = 2 * SIMULATION_VELOCITY_MEAN;
        constexpr filters::NoiseModel<T> FILTER_NOISE_MODEL =
                filters::DiscreteNoiseModel<T>{.variance = SIMULATION_VELOCITY_VARIANCE};
        constexpr filters::NoiseModel<T> FILTER_INFO_NOISE_MODEL =
                filters::ContinuousNoiseModel<T>{.spectral_density = SIMULATION_DT * SIMULATION_VELOCITY_VARIANCE};
        constexpr T FILTER_NO_FADING_MEMORY = 1;
        constexpr T FILTER_FADING_MEMORY_ALPHA = 1.01;
        constexpr T FILTER_RESET_DT = 10;
        constexpr std::optional<T> FILTER_GATE{};

        const std::vector<Measurements<T>> measurements = simulator::simulate_speed<T>(
                SIMULATION_LENGTH, SIMULATION_INIT_X, SIMULATION_DT, SIMULATION_VELOCITY_MEAN,
                SIMULATION_VELOCITY_VARIANCE, SIMULATION_MEASUREMENT_VARIANCE_X, SIMULATION_MEASUREMENT_VARIANCE_V);

        const std::vector<unsigned> distribution = {580, 230, 60, 16, 7, 3, 0, 0, 0, 0};
        const std::array<T, 2> min_max_nees_x{0.4, 1.0};
        const std::array<T, 2> min_max_nees_xv{0.15, 2.95};

        const std::string annotation = make_annotation(
                SIMULATION_DT, SIMULATION_VELOCITY_VARIANCE, SIMULATION_MEASUREMENT_VARIANCE_X,
                SIMULATION_MEASUREMENT_VARIANCE_V);

        //

        test_impl<T>(
                "EKF", annotation,
                filters::create_ekf<T>(
                        FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_NOISE_MODEL, FILTER_NO_FADING_MEMORY,
                        FILTER_RESET_DT, FILTER_GATE),
                measurements, precision_x, precision_xv, 1.4306576889002234962L, 0.298852051973191582294L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);

        test_impl<T>(
                "H_INFINITY", annotation,
                filters::create_h_infinity<T>(
                        FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_NOISE_MODEL, FILTER_NO_FADING_MEMORY,
                        FILTER_RESET_DT, FILTER_GATE),
                measurements, precision_x, precision_xv, 1.43098764352003224212L, 0.298852351037763028539L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);

        test_impl<T>(
                "INFO", annotation,
                filters::create_info<T>(
                        FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_INFO_NOISE_MODEL, FILTER_NO_FADING_MEMORY,
                        FILTER_RESET_DT, FILTER_GATE),
                measurements, precision_x, precision_xv, 1.43109224963343917639L, 0.351851021981079359921L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);

        test_impl<T>(
                "UKF", annotation,
                filters::create_ukf<T>(
                        FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_NOISE_MODEL, FILTER_NO_FADING_MEMORY,
                        FILTER_RESET_DT, FILTER_GATE),
                measurements, precision_x, precision_xv, 1.43670888967218343853L, 0.304462860876562311786L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);

        //

        test_impl<T>(
                "EKF_FM", annotation,
                filters::create_ekf<T>(
                        FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_NOISE_MODEL, FILTER_FADING_MEMORY_ALPHA,
                        FILTER_RESET_DT, FILTER_GATE),
                measurements, precision_x, precision_xv, 1.47717680187677689158L, 0.46227454729500716218L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);

        test_impl<T>(
                "H_INFINITY_FM", annotation,
                filters::create_h_infinity<T>(
                        FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_NOISE_MODEL, FILTER_FADING_MEMORY_ALPHA,
                        FILTER_RESET_DT, FILTER_GATE),
                measurements, precision_x, precision_xv, 1.47758200411625906223L, 0.462276094360566754132L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);

        test_impl<T>(
                "INFO_FM", annotation,
                filters::create_info<T>(
                        FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_INFO_NOISE_MODEL, FILTER_FADING_MEMORY_ALPHA,
                        FILTER_RESET_DT, FILTER_GATE),
                measurements, precision_x, precision_xv, 1.47758703673015917579L, 0.489399903037307279361L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);

        test_impl<T>(
                "UKF_FM", annotation,
                filters::create_ukf<T>(
                        FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_NOISE_MODEL, FILTER_FADING_MEMORY_ALPHA,
                        FILTER_RESET_DT, FILTER_GATE),
                measurements, precision_x, precision_xv, 1.51047329031311578808L, 0.483217371469443008448L, 5,
                distribution, min_max_nees_x, min_max_nees_xv);
}

void test()
{
        LOG("Test Filters");
        test_impl<float>(1e-3, 5e-3);
        test_impl<double>(2e-12, 5e-12);
        test_impl<long double>(4e-16, 3e-15);
        LOG("Test Filters passed");
}

TEST_SMALL("Filters", test)
}
}
