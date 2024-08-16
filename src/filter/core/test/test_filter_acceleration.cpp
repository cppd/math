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

#include "filters/filter.h"
#include "filters/noise_model.h"
#include "simulator/acceleration.h"
#include "simulator/measurements.h"
#include "view/write.h"

#include <src/color/rgb8.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/test/test.h>

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ns::filter::core::test
{
namespace
{
constexpr std::string_view SIGMA = "&#x03c3;";

template <typename T>
constexpr T DATA_CONNECT_INTERVAL = 10;

template <typename T>
struct SimulationConfig final
{
        T length = 500;
        T dt = 0.5;
        T acceleration = 2;
        T velocity_variance = square(0.1);
        T measurement_variance_x = square(100.0);
        T measurement_variance_v = square(0.5);
        T init_x = 0;
};

template <typename T>
struct FilterConfig final
{
        T init_v = 0;
        T init_v_variance = square(10.0);
        T reset_dt = 20;
        std::optional<T> gate{5};
        filters::DiscreteNoiseModel<T> discrete_noise{.variance = square(2)};
        filters::ContinuousNoiseModel<T> continuous_noise{.spectral_density = 2 * discrete_noise.variance};
        T fading_memory_alpha = 1.005;
};

template <typename T>
std::string make_annotation(
        const SimulationConfig<T>& simulation_config,
        const FilterConfig<T>& filter_config,
        const simulator::MeasurementConfig<T>& measurement_config)
{
        std::ostringstream oss;

        oss << "<b>update</b>";
        oss << "<br>";
        oss << "position: " << 1 / measurement_config.position_reset_interval << " Hz";
        oss << "<br>";
        oss << "speed: " << 1 / simulation_config.dt << " Hz";

        oss << "<br>";
        oss << "<br>";
        oss << "<b>" << SIGMA << "</b>";
        oss << "<br>";
        oss << "process speed: " << std::sqrt(simulation_config.velocity_variance) << " m/s";
        oss << "<br>";
        oss << "position: " << std::sqrt(simulation_config.measurement_variance_x) << " m";
        oss << "<br>";
        oss << "speed: " << std::sqrt(simulation_config.measurement_variance_v) << " m/s";

        oss << "<br>";
        oss << "<br>";
        oss << "<b>settings</b>";
        oss << "<br>";
        oss << "speed factor: " << measurement_config.speed_factor;
        oss << "<br>";
        oss << "acceleration: " << simulation_config.acceleration << " m/s<sup>2</sup>";
        oss << "<br>";
        oss << "filter " << SIGMA << ": " << std::sqrt(filter_config.discrete_noise.variance);
        oss << "<br>";
        oss << "filter " << SIGMA << " interval: "
            << filter_config.continuous_noise.spectral_density / filter_config.discrete_noise.variance << " s";
        oss << "<br>";
        oss << "filter gate: ";
        if (filter_config.gate)
        {
                oss << *filter_config.gate;
        }
        else
        {
                oss << "none";
        }

        return oss.str();
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
        const std::vector<Measurements<T>>& measurements,
        simulator::VarianceCorrection<T>* const correction)
{
        filter->reset();
        correction->reset();

        std::vector<view::Point<T>> res;
        for (Measurements<T> m : measurements)
        {
                correction->correct(&m);

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
        const FilterConfig<T>& config,
        const std::vector<Measurements<T>>& measurements,
        simulator::VarianceCorrection<T>* const correction)
{
        const auto positions = reset_v(measurements);

        const auto ekf_continuous = filters::create_ekf<T>(
                config.init_v, config.init_v_variance, config.continuous_noise, config.fading_memory_alpha,
                config.reset_dt, config.gate);

        const auto ekf_discrete = filters::create_ekf<T>(
                config.init_v, config.init_v_variance, config.discrete_noise, config.fading_memory_alpha,
                config.reset_dt, config.gate);

        const auto info_continuous = filters::create_info<T>(
                config.init_v, config.init_v_variance, config.continuous_noise, config.fading_memory_alpha,
                config.reset_dt, config.gate);

        std::vector<view::Filter<T>> filters;

        filters.emplace_back(
                "C Positions", color::RGB8(180, 0, 0), test_filter(ekf_continuous.get(), positions, correction));
        filters.emplace_back(
                "C Measurements", color::RGB8(0, 180, 0), test_filter(ekf_continuous.get(), measurements, correction));

        filters.emplace_back(
                "D Positions", color::RGB8(128, 0, 0), test_filter(ekf_discrete.get(), positions, correction));
        filters.emplace_back(
                "D Measurements", color::RGB8(0, 128, 0), test_filter(ekf_discrete.get(), measurements, correction));

        filters.emplace_back(
                "I Positions", color::RGB8(230, 0, 0), test_filter(info_continuous.get(), positions, correction));
        filters.emplace_back(
                "I Measurements", color::RGB8(0, 230, 0), test_filter(info_continuous.get(), measurements, correction));

        view::write(name, annotation, measurements, DATA_CONNECT_INTERVAL<T>, filters);
}

template <typename T>
void test_impl()
{
        const SimulationConfig<T> simulation_config;
        const FilterConfig<T> filter_config;

        const simulator::SimulatorMeasurements<T> measurements =
                simulator::prepare_measurements(simulator::simulate_acceleration<T>(
                        simulation_config.length, simulation_config.init_x, simulation_config.dt,
                        simulation_config.acceleration, simulation_config.velocity_variance,
                        simulation_config.measurement_variance_x, simulation_config.measurement_variance_v));
        ASSERT(measurements.variance_correction);

        const std::string annotation = make_annotation(simulation_config, filter_config, measurements.config);

        test_impl<T>(
                "acceleration", annotation, filter_config, measurements.measurements,
                measurements.variance_correction.get());
}

void test()
{
        LOG("Test Filter Acceleration");
        test_impl<float>();
        test_impl<double>();
        test_impl<long double>();
        LOG("Test Filter Acceleration passed");
}

TEST_SMALL("Filter Acceleration", test)
}
}
