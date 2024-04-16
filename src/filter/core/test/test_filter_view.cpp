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
#include "view/write.h"

#include <src/color/rgb8.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/test/test.h>

#include <cstddef>
#include <memory>
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
        const unsigned interval)
{
        std::vector<Measurements<T>> res(measurements);
        for (std::size_t i = 0; i < res.size(); ++i)
        {
                if (i % interval != 0)
                {
                        res[i].x.reset();
                }
                if (i >= 450 && i < 570)
                {
                        res[i].x.reset();
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
                res.push_back({.time = m.time, .x = update->x, .stddev = update->stddev});
        }
        return res;
}

template <typename T>
void test_impl(
        const std::string_view name,
        std::unique_ptr<filters::Filter<T>> filter,
        const std::vector<Measurements<T>>& measurements)
{
        const std::vector<view::Point<T>> x = test_filter(filter.get(), reset_v(measurements));

        const std::vector<view::Point<T>> xv = test_filter(filter.get(), measurements);

        view::write(
                name, measurements, DATA_CONNECT_INTERVAL<T>,
                {view::Filter<T>("Position", color::RGB8(128, 0, 0), x),
                 view::Filter<T>("Position Speed", color::RGB8(0, 128, 0), xv)});
}

template <typename T>
void test_impl()
{
        constexpr std::size_t SIMULATION_COUNT = 1000;

        constexpr T POSITION_MEASUREMENTS_RESET_INTERVAL = 5;
        constexpr T SIMULATION_DT = 0.5;

        constexpr T SIMULATION_ACCELERATION = 2;
        constexpr T SIMULATION_VELOCITY_VARIANCE = square(0.2);
        constexpr T SIMULATION_MEASUREMENT_VARIANCE_X = square(100);
        constexpr T SIMULATION_MEASUREMENT_VARIANCE_V = square(0.2);
        constexpr T SIMULATION_INIT_X = 0;

        constexpr T FILTER_INIT_V = 0;
        constexpr T FILTER_INIT_V_VARIANCE = square(10.0);
        constexpr T FILTER_VELOCITY_VARIANCE = 2;

        const std::vector<Measurements<T>> measurements = simulate_acceleration<T>(
                SIMULATION_COUNT, SIMULATION_INIT_X, SIMULATION_DT, SIMULATION_ACCELERATION,
                SIMULATION_VELOCITY_VARIANCE, SIMULATION_MEASUREMENT_VARIANCE_X, SIMULATION_MEASUREMENT_VARIANCE_V);

        test_impl<T>(
                "view", filters::create_ukf<T>(FILTER_INIT_V, FILTER_INIT_V_VARIANCE, FILTER_VELOCITY_VARIANCE),
                reset_position_measurements(measurements, POSITION_MEASUREMENTS_RESET_INTERVAL));
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
