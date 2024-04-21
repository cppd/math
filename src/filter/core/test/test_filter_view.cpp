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
std::vector<Measurements<T>> add_bad_measurements(const std::vector<Measurements<T>>& measurements)
{
        constexpr T X = 1000;
        constexpr T V = 50;
        constexpr T PROBABILITY = T{1} / 20;

        PCG engine;

        std::vector<Measurements<T>> res(measurements);
        for (Measurements<T>& m : std::ranges::drop_view(res, 5))
        {
                if (m.x && std::bernoulli_distribution(PROBABILITY)(engine))
                {
                        m.x->value += std::bernoulli_distribution(0.5)(engine) ? X : -X;
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
        const T init_v,
        const T init_v_variance,
        const std::optional<T> gate,
        const std::vector<Measurements<T>>& measurements)
{
        using C = filters::ContinuousNoiseModel<T>;
        using D = filters::DiscreteNoiseModel<T>;

        const auto position_measurements = reset_v(measurements);

        constexpr T SIGMA = 2;
        constexpr T INTERVAL = 2.5;

        const auto f_c =
                filters::create_ekf<T>(init_v, init_v_variance, C{.spectral_density = INTERVAL * square(SIGMA)}, gate);
        const auto f_d = filters::create_ekf<T>(init_v, init_v_variance, D{.variance = square(SIGMA)}, gate);

        view::write(
                name, measurements, DATA_CONNECT_INTERVAL<T>,
                {view::Filter<T>("C Position", color::RGB8(180, 0, 0), test_filter(f_c.get(), position_measurements)),
                 view::Filter<T>("C Speed", color::RGB8(0, 180, 0), test_filter(f_c.get(), measurements)),
                 view::Filter<T>("D Position", color::RGB8(128, 0, 0), test_filter(f_d.get(), position_measurements)),
                 view::Filter<T>("D Speed", color::RGB8(0, 128, 0), test_filter(f_d.get(), measurements))});
}

template <typename T>
void test_impl()
{
        constexpr T SIMULATION_LENGTH = 500;

        constexpr T POSITION_MEASUREMENTS_RESET_INTERVAL = 2.5;
        constexpr T SIMULATION_DT = 0.5;

        constexpr T SIMULATION_ACCELERATION = 2;
        constexpr T SIMULATION_VELOCITY_VARIANCE = square(0.1);
        constexpr T SIMULATION_MEASUREMENT_VARIANCE_X = square(100.0);
        constexpr T SIMULATION_MEASUREMENT_VARIANCE_V = square(0.2);
        constexpr T SIMULATION_INIT_X = 0;

        constexpr T FILTER_INIT_V = 0;
        constexpr T FILTER_INIT_V_VARIANCE = square(10.0);
        constexpr std::optional<T> FILTER_GATE{5};

        const std::vector<Measurements<T>> measurements = simulate_acceleration<T>(
                SIMULATION_LENGTH, SIMULATION_INIT_X, SIMULATION_DT, SIMULATION_ACCELERATION,
                SIMULATION_VELOCITY_VARIANCE, SIMULATION_MEASUREMENT_VARIANCE_X, SIMULATION_MEASUREMENT_VARIANCE_V);

        test_impl<T>(
                "view", FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_GATE,
                add_bad_measurements(reset_position_measurements(measurements, POSITION_MEASUREMENTS_RESET_INTERVAL)));
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
