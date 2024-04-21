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

#include "measurements.h"
#include "simulator.h"

#include "filters/filter.h"
#include "filters/noise_model.h"
#include "view/write.h"

#include <src/color/rgb8.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/random/pcg.h>
#include <src/test/test.h>

#include <optional>
#include <random>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ns::filter::core::test
{
namespace
{
template <typename T>
constexpr T DATA_CONNECT_INTERVAL = 10;

template <typename T>
std::vector<Measurements<T>> reset_position_measurements(
        const std::vector<Measurements<T>>& measurements,
        const T interval)
{
        if (measurements.empty())
        {
                return {};
        }

        std::vector<Measurements<T>> res(measurements);
        auto iter = res.begin();
        T next_time = iter->time + interval;
        while (++iter != res.end())
        {
                if (iter->time < next_time)
                {
                        iter->x.reset();
                }
                else
                {
                        next_time = iter->time + interval;
                }
                if (iter->time >= 225 && iter->time < 285)
                {
                        iter->x.reset();
                }
        }
        return res;
}

template <typename T>
std::vector<Measurements<T>> add_bad_measurements(
        const std::vector<Measurements<T>>& measurements,
        const T speed_factor)
{
        constexpr T X = 2000;
        constexpr T V = 30;
        constexpr T PROBABILITY = T{1} / 20;

        PCG engine;

        std::vector<Measurements<T>> res(measurements);
        for (Measurements<T>& m : std::ranges::drop_view(res, 5))
        {
                if (m.x && std::bernoulli_distribution(PROBABILITY)(engine))
                {
                        m.x->value += std::bernoulli_distribution(0.5)(engine) ? X : -X;
                }
                if (m.v)
                {
                        m.v->value *= speed_factor;
                }
                if (m.v && std::bernoulli_distribution(PROBABILITY)(engine))
                {
                        m.v->value += V;
                }
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
std::vector<view::Point<T>> test_filter(
        filters::Filter<T>* const filter,
        const std::vector<Measurements<T>>& measurements)
{
        filter->reset();
        std::vector<view::Point<T>> res;
        for (const Measurements<T>& m : measurements)
        {
                const auto update = filter->update(m);
                if (!update)
                {
                        continue;
                }
                res.push_back(
                        {.time = m.time,
                         .x = update->x,
                         .x_stddev = update->x_stddev,
                         .v = update->v,
                         .v_stddev = update->v_stddev});
        }
        return res;
}

template <typename T>
void test_impl(
        const std::string_view name,
        const std::string_view annotation,
        const T init_v,
        const T init_v_variance,
        const std::optional<T> gate,
        const T sigma,
        const T sigma_interval,
        const std::vector<Measurements<T>>& measurements)
{
        using C = filters::ContinuousNoiseModel<T>;
        using D = filters::DiscreteNoiseModel<T>;

        const auto position_measurements = reset_v(measurements);

        const auto f_c = filters::create_ekf<T>(
                init_v, init_v_variance, C{.spectral_density = sigma_interval * square(sigma)}, gate);

        const auto f_d = filters::create_ekf<T>(init_v, init_v_variance, D{.variance = square(sigma)}, gate);

        view::write(
                name, annotation, measurements, DATA_CONNECT_INTERVAL<T>,
                {view::Filter<T>("C Position", color::RGB8(180, 0, 0), test_filter(f_c.get(), position_measurements)),
                 view::Filter<T>("C Speed", color::RGB8(0, 180, 0), test_filter(f_c.get(), measurements)),
                 view::Filter<T>("D Position", color::RGB8(128, 0, 0), test_filter(f_d.get(), position_measurements)),
                 view::Filter<T>("D Speed", color::RGB8(0, 128, 0), test_filter(f_d.get(), measurements))});
}

template <typename T>
std::string make_annotation(
        const T position_measurements_reset_interval,
        const T simulation_dt,
        const T simulation_acceleration,
        const T simulation_velocity_variance,
        const T simulation_measurement_variance_x,
        const T simulation_measurement_variance_v,
        const T measurement_speed_factor,
        const std::optional<T> filter_gate,
        const T filter_sigma,
        const T filter_sigma_interval)
{
        constexpr std::string_view SIGMA = "&#x03c3;";

        std::ostringstream oss;

        oss << "<b>update</b>";
        oss << "<br>";
        oss << "position: " << 1 / position_measurements_reset_interval << " Hz";
        oss << "<br>";
        oss << "speed: " << 1 / simulation_dt << " Hz";

        oss << "<br>";
        oss << "<br>";
        oss << "<b>" << SIGMA << "</b>";
        oss << "<br>";
        oss << "process speed: " << std::sqrt(simulation_velocity_variance) << " m/s";
        oss << "<br>";
        oss << "position: " << std::sqrt(simulation_measurement_variance_x) << " m";
        oss << "<br>";
        oss << "speed: " << std::sqrt(simulation_measurement_variance_v) << " m/s";

        oss << "<br>";
        oss << "<br>";
        oss << "<b>settings</b>";
        oss << "<br>";
        oss << "speed factor: " << measurement_speed_factor;
        oss << "<br>";
        oss << "acceleration: " << simulation_acceleration << " m/s<sup>2</sup>";
        oss << "<br>";
        oss << "filter " << SIGMA << ": " << filter_sigma;
        oss << "<br>";
        oss << "filter " << SIGMA << " interval: " << filter_sigma_interval << " s";
        oss << "<br>";
        oss << "filter gate: ";
        if (filter_gate)
        {
                oss << *filter_gate;
        }
        else
        {
                oss << "none";
        }

        return oss.str();
}

template <typename T>
void test_impl()
{
        constexpr T SIMULATION_LENGTH = 500;

        constexpr T POSITION_MEASUREMENTS_RESET_INTERVAL = 2;
        constexpr T SIMULATION_DT = 0.5;

        constexpr T SIMULATION_ACCELERATION = 2;
        constexpr T SIMULATION_VELOCITY_VARIANCE = square(0.1);
        constexpr T SIMULATION_MEASUREMENT_VARIANCE_X = square(100.0);
        constexpr T SIMULATION_MEASUREMENT_VARIANCE_V = square(0.5);
        constexpr T SIMULATION_INIT_X = 0;

        constexpr T MEASUREMENT_SPEED_FACTOR = 1;

        constexpr T FILTER_INIT_V = 0;
        constexpr T FILTER_INIT_V_VARIANCE = square(10.0);
        constexpr std::optional<T> FILTER_GATE{5};

        constexpr T FILTER_SIGMA = 1;
        constexpr T FILTER_SIGMA_INTERVAL = 2;

        const std::vector<Measurements<T>> measurements = simulate_acceleration<T>(
                SIMULATION_LENGTH, SIMULATION_INIT_X, SIMULATION_DT, SIMULATION_ACCELERATION,
                SIMULATION_VELOCITY_VARIANCE, SIMULATION_MEASUREMENT_VARIANCE_X, SIMULATION_MEASUREMENT_VARIANCE_V);

        const std::string annotation = make_annotation(
                POSITION_MEASUREMENTS_RESET_INTERVAL, SIMULATION_DT, SIMULATION_ACCELERATION,
                SIMULATION_VELOCITY_VARIANCE, SIMULATION_MEASUREMENT_VARIANCE_X, SIMULATION_MEASUREMENT_VARIANCE_V,
                MEASUREMENT_SPEED_FACTOR, FILTER_GATE, FILTER_SIGMA, FILTER_SIGMA_INTERVAL);

        test_impl<T>(
                "view", annotation, FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_GATE, FILTER_SIGMA,
                FILTER_SIGMA_INTERVAL,
                add_bad_measurements(
                        reset_position_measurements(measurements, POSITION_MEASUREMENTS_RESET_INTERVAL),
                        MEASUREMENT_SPEED_FACTOR));
}

void test()
{
        LOG("Test Filter View");
        test_impl<float>();
        test_impl<double>();
        test_impl<long double>();
        LOG("Test Filter View passed");
}

TEST_SMALL("Filter View", test)
}
}
