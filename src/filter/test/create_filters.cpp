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

#include "create_filters.h"

#include "config.h"

#include "view/write.h"

#include <src/color/rgb8.h>
#include <src/com/error.h>
#include <src/com/sort.h>
#include <src/filter/filters/acceleration/acceleration.h>
#include <src/filter/filters/direction/direction.h>
#include <src/filter/filters/estimation/position_estimation.h>
#include <src/filter/filters/estimation/position_variance.h>
#include <src/filter/filters/position/position.h>
#include <src/filter/filters/speed/speed.h>
#include <src/filter/utility/instantiation.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

namespace ns::filter::test
{
namespace
{
const auto* const ALPHA = reinterpret_cast<const char*>(u8"\u03b1");
const auto* const THETA = reinterpret_cast<const char*>(u8"\u03b8");

template <std::size_t N, typename T>
[[nodiscard]] int compute_string_precision(const std::array<T, N>& data)
{
        std::optional<T> min;

        for (const T v : data)
        {
                ASSERT(v >= 0);
                if (!(v > 0))
                {
                        continue;
                }
                if (min)
                {
                        min = std::min(v, *min);
                        continue;
                }
                min = v;
        }

        if (!min)
        {
                return 0;
        }

        ASSERT(*min >= 1e-6L);
        return std::abs(std::floor(std::log10(*min)));
}

template <std::size_t N, typename T>
std::unique_ptr<filters::estimation::PositionVariance<N, T>> create_position_variance(
        const PositionVarianceConfig<T>& config)
{
        return std::make_unique<filters::estimation::PositionVariance<N, T>>(
                config.reset_dt, config.noise_model_2, config.fading_memory_alpha_2, config.init);
}

template <std::size_t N, typename T, std::size_t ORDER>
TestFilterPosition<N, T> create_position(const PositionConfig<T>& config, const unsigned i, const T theta)
{
        ASSERT(theta >= 0 && theta <= 1);
        ASSERT(i <= 4);

        const int precision = compute_string_precision(config.thetas);

        const auto name = [&]
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Position " << ORDER << " (" << THETA << " " << theta << ")";
                return oss.str();
        }();

        static_assert(ORDER >= 0 && ORDER <= 2);
        switch (ORDER)
        {
        case 0:
                return {filters::position::create_position_0<N, T>(
                                config.reset_dt, config.linear_dt, config.gate_0, config.init, theta,
                                config.noise_model_0, config.fading_memory_alpha_0),
                        view::Filter<N, T>(name, color::RGB8(160 - 40 * i, 100, 200))};
        case 1:
                return {filters::position::create_position_1<N, T>(
                                config.reset_dt, config.linear_dt, config.gate_1, config.init, theta,
                                config.noise_model_1, config.fading_memory_alpha_1),
                        view::Filter<N, T>(name, color::RGB8(160 - 40 * i, 0, 200))};
        case 2:
                return {filters::position::create_position_2<N, T>(
                                config.reset_dt, config.linear_dt, config.gate_2, config.init, theta,
                                config.noise_model_2, config.fading_memory_alpha_2),
                        view::Filter<N, T>(name, color::RGB8(160 - 40 * i, 0, 0))};
        default:
                ASSERT(false);
        }
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<TestFilterPosition<N, T>> create_positions(const PositionConfig<T>& config)
{
        std::vector<TestFilterPosition<N, T>> res;

        const auto thetas = sort(std::array(config.thetas));

        res.reserve(thetas.size());

        for (std::size_t i = 0; i < thetas.size(); ++i)
        {
                res.push_back(create_position<N, T, ORDER>(config, i, thetas[i]));
        }

        return res;
}

template <typename T, std::size_t ORDER>
TestFilter<2, T> create_acceleration(const AccelerationConfig<T>& config, const unsigned i, const T alpha)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 4);

        const int precision = compute_string_precision(config.ukf_alphas);

        const auto name = [&]
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Acceleration " << ORDER << " (" << ALPHA << " " << alpha << ")";
                return oss.str();
        }();

        static_assert(ORDER == 0 || ORDER == 1);
        switch (ORDER)
        {
        case 0:
                return {filters::acceleration::create_acceleration_0<T>(
                                config.measurement_queue_size, config.reset_dt, config.angle_estimation_variance,
                                config.gate, config.init, alpha, config.position_noise_model,
                                config.angle_noise_model_0, config.angle_r_noise_model_0, config.fading_memory_alpha_0),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 0))};
        case 1:
                return {filters::acceleration::create_acceleration_1<T>(
                                config.measurement_queue_size, config.reset_dt, config.angle_estimation_variance,
                                config.gate, config.init, alpha, config.position_noise_model,
                                config.angle_noise_model_1, config.angle_r_noise_model_1, config.fading_memory_alpha_1),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 0))};
        default:
                ASSERT(false);
        }
}

template <typename T>
std::vector<TestFilter<2, T>> create_accelerations(const AccelerationConfig<T>& config)
{
        std::vector<TestFilter<2, T>> res;

        const auto alphas = sort(std::array(config.ukf_alphas));

        res.reserve(1 + 2 * alphas.size());

        res.emplace_back(
                filters::acceleration::create_acceleration_ekf<T>(
                        config.measurement_queue_size, config.reset_dt, config.angle_estimation_variance, config.gate,
                        config.init, config.position_noise_model, config.angle_noise_model_1,
                        config.angle_r_noise_model_1, config.fading_memory_alpha_1),
                view::Filter<2, T>("Acceleration EKF", color::RGB8(0, 200, 0)));

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_acceleration<T, 0>(config, i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_acceleration<T, 1>(config, i, alphas[i]));
        }

        return res;
}

template <typename T, std::size_t ORDER>
TestFilter<2, T> create_direction(const DirectionConfig<T>& config, const unsigned i, const T alpha)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 4);

        const int precision = compute_string_precision(config.ukf_alphas);

        const auto name = [&]
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Direction " << ORDER / 10 << '.' << ORDER % 10 << " (" << ALPHA << " " << alpha << ")";
                return oss.str();
        }();

        static_assert(ORDER == 10 || ORDER == 11 || ORDER == 21);
        switch (ORDER)
        {
        case 10:
                return {filters::direction::create_direction_1_0<T>(
                                config.measurement_queue_size, config.reset_dt, config.angle_estimation_variance,
                                config.gate, config.init, alpha, config.position_noise_model_1_0,
                                config.angle_noise_model_1_0, config.fading_memory_alpha_1_0),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 250))};
        case 11:
                return {filters::direction::create_direction_1_1<T>(
                                config.measurement_queue_size, config.reset_dt, config.angle_estimation_variance,
                                config.gate, config.init, alpha, config.position_noise_model_1_1,
                                config.angle_noise_model_1_1, config.fading_memory_alpha_1_1),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 150))};
        case 21:
                return {filters::direction::create_direction_2_1<T>(
                                config.measurement_queue_size, config.reset_dt, config.angle_estimation_variance,
                                config.gate, config.init, alpha, config.position_noise_model_2_1,
                                config.angle_noise_model_2_1, config.fading_memory_alpha_2_1),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 50))};
        default:
                ASSERT(false);
        }
}

template <typename T>
std::vector<TestFilter<2, T>> create_directions(const DirectionConfig<T>& config)
{
        std::vector<TestFilter<2, T>> res;

        const auto alphas = sort(std::array(config.ukf_alphas));

        res.reserve(3 * alphas.size());

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 10>(config, i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 11>(config, i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 21>(config, i, alphas[i]));
        }

        return res;
}

template <typename T, std::size_t ORDER_P>
TestFilter<2, T> create_speed(const SpeedConfig<T>& config, const unsigned i, const T alpha)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 2);

        const int precision = compute_string_precision(config.ukf_alphas);

        const auto name = [&]
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Speed " << ORDER_P << " (" << ALPHA << " " << alpha << ")";
                return oss.str();
        }();

        static_assert(ORDER_P == 1 || ORDER_P == 2);
        switch (ORDER_P)
        {
        case 1:
                return {filters::speed::create_speed_1<2, T>(
                                config.measurement_queue_size, config.reset_dt, config.angle_estimation_variance,
                                config.gate, config.init, alpha, config.noise_model_1, config.fading_memory_alpha_1),
                        view::Filter<2, T>(name, color::RGB8(0, 200 - 40 * i, 0))};
        case 2:
                return {filters::speed::create_speed_2<2, T>(
                                config.measurement_queue_size, config.reset_dt, config.angle_estimation_variance,
                                config.gate, config.init, alpha, config.noise_model_2, config.fading_memory_alpha_2),
                        view::Filter<2, T>(name, color::RGB8(0, 150 - 40 * i, 0))};
        default:
                ASSERT(false);
        }
}

template <typename T>
std::vector<TestFilter<2, T>> create_speeds(const SpeedConfig<T>& config)
{
        std::vector<TestFilter<2, T>> res;

        const auto alphas = sort(std::array(config.ukf_alphas));

        res.reserve(2 * alphas.size());

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_speed<T, 1>(config, i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_speed<T, 2>(config, i, alphas[i]));
        }

        return res;
}
}

template <typename T>
Filters<T> create_filters()
{
        const PositionVarianceConfig<T> position_variance;
        const PositionConfig<T> position;
        const AccelerationConfig<T> acceleration;
        const DirectionConfig<T> direction;
        const SpeedConfig<T> speed;

        Filters<T> res;

        res.position_variance = create_position_variance<2, T>(position_variance);

        res.positions_0 = create_positions<2, T, 0>(position);
        res.positions_1 = create_positions<2, T, 1>(position);
        res.positions_2 = create_positions<2, T, 2>(position);

        res.accelerations = create_accelerations<T>(acceleration);
        res.directions = create_directions<T>(direction);
        res.speeds = create_speeds<T>(speed);

        res.position_estimation =
                std::make_unique<filters::estimation::PositionEstimation<2, T>>(res.positions_2.front().filter.get());

        return res;
}

#define TEMPLATE(T) template Filters<T> create_filters();

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
