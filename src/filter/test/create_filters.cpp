/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../filters/acceleration/acceleration_ekf.h"
#include "../filters/acceleration/acceleration_ukf.h"
#include "../filters/direction/direction_1_0.h"
#include "../filters/direction/direction_1_1.h"
#include "../filters/direction/direction_2_1.h"
#include "../filters/position/position_0.h"
#include "../filters/position/position_1.h"
#include "../filters/position/position_2.h"
#include "../filters/speed/speed_1.h"
#include "../filters/speed/speed_2.h"
#include "../settings/instantiation.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/sort.h>

#include <array>
#include <cmath>
#include <optional>
#include <sstream>

namespace ns::filter::test
{
namespace
{
const auto* const ALPHA = reinterpret_cast<const char*>(u8"\u03b1");
const auto* const THETA = reinterpret_cast<const char*>(u8"\u03b8");

template <typename T>
struct Config final
{
        static constexpr T POSITION_FILTER_VARIANCE_0 = square(0.5);
        static constexpr std::optional<T> POSITION_FILTER_GATE_0{};

        static constexpr T POSITION_FILTER_VARIANCE_1 = square(1);
        static constexpr std::optional<T> POSITION_FILTER_GATE_1{10};

        static constexpr T POSITION_FILTER_VARIANCE_2 = square(0.5);
        static constexpr std::optional<T> POSITION_FILTER_GATE_2{5};

        static constexpr std::array POSITION_FILTER_THETAS = std::to_array<T>({0});
        static constexpr T POSITION_FILTER_RESET_DT = 10;
        static constexpr T POSITION_FILTER_LINEAR_DT = 2;
        static constexpr filters::position::Init<T> POSITION_INIT{
                .speed = 0,
                .speed_variance = square<T>(30),
                .acceleration = 0,
                .acceleration_variance = square<T>(10)};
        static constexpr filters::position::Init<T> POSITION_VARIANCE_INIT{
                .speed = 0,
                .speed_variance = square<T>(30),
                .acceleration = 0,
                .acceleration_variance = square<T>(10)};

        static constexpr T ACCELERATION_FILTER_POSITION_VARIANCE = square(1.0);
        static constexpr T ACCELERATION_FILTER_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T ACCELERATION_FILTER_ANGLE_R_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T ACCELERATION_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr std::array ACCELERATION_FILTER_UKF_ALPHAS = std::to_array<T>({0.1, 1.0});
        static constexpr T ACCELERATION_FILTER_RESET_DT = 10;
        static constexpr std::optional<T> ACCELERATION_FILTER_GATE{};
        static constexpr filters::acceleration::Init<T> ACCELERATION_INIT{
                .angle = 0,
                .angle_variance = square(degrees_to_radians(100.0)),
                .acceleration = 0,
                .acceleration_variance = square(10),
                .angle_speed = 0,
                .angle_speed_variance = square(degrees_to_radians(1.0)),
                .angle_r = 0,
                .angle_r_variance = square(degrees_to_radians(50.0))};
        static constexpr std::size_t ACCELERATION_MEASUREMENT_QUEUE_SIZE = 20;

        static constexpr T DIRECTION_FILTER_POSITION_VARIANCE_1_0 = square(2.0);
        static constexpr T DIRECTION_FILTER_POSITION_VARIANCE_1_1 = square(2.0);
        static constexpr T DIRECTION_FILTER_POSITION_VARIANCE_2_1 = square(1.0);
        static constexpr T DIRECTION_FILTER_ANGLE_VARIANCE_1_0 = square(degrees_to_radians(0.2));
        static constexpr T DIRECTION_FILTER_ANGLE_VARIANCE_1_1 = square(degrees_to_radians(0.001));
        static constexpr T DIRECTION_FILTER_ANGLE_VARIANCE_2_1 = square(degrees_to_radians(0.001));
        static constexpr T DIRECTION_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr std::array DIRECTION_FILTER_UKF_ALPHAS = std::to_array<T>({1.0});
        static constexpr T DIRECTION_FILTER_RESET_DT = 10;
        static constexpr std::optional<T> DIRECTION_FILTER_GATE{};
        static constexpr filters::direction::Init<T> DIRECTION_INIT{
                .angle = 0,
                .angle_variance = square(degrees_to_radians(100.0)),
                .acceleration = 0,
                .acceleration_variance = square(10),
                .angle_speed = 0,
                .angle_speed_variance = square(degrees_to_radians(1.0))};
        static constexpr std::size_t DIRECTION_MEASUREMENT_QUEUE_SIZE = 20;

        static constexpr T SPEED_FILTER_POSITION_VARIANCE_1 = square(2.0);
        static constexpr T SPEED_FILTER_POSITION_VARIANCE_2 = square(2.0);
        static constexpr T SPEED_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr std::array SPEED_FILTER_UKF_ALPHAS = std::to_array<T>({1.0});
        static constexpr T SPEED_FILTER_RESET_DT = 10;
        static constexpr std::optional<T> SPEED_FILTER_GATE{};
        static constexpr filters::speed::Init<T> SPEED_INIT{.acceleration = 0, .acceleration_variance = square(10)};
        static constexpr std::size_t SPEED_MEASUREMENT_QUEUE_SIZE = 20;
};

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
std::unique_ptr<filters::estimation::PositionVariance<N, T>> create_position_variance()
{
        return std::make_unique<filters::estimation::PositionVariance<N, T>>(
                Config<T>::POSITION_FILTER_RESET_DT, Config<T>::POSITION_FILTER_VARIANCE_2,
                Config<T>::POSITION_VARIANCE_INIT);
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<TestFilterPosition<N, T>> create_positions()
{
        std::vector<TestFilterPosition<N, T>> res;

        const int precision = compute_string_precision(Config<T>::POSITION_FILTER_THETAS);

        const auto name = [&](const T theta)
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Position " << ORDER << " (" << THETA << " " << theta << ")";
                return oss.str();
        };

        const auto thetas = sort(std::array(Config<T>::POSITION_FILTER_THETAS));
        for (std::size_t i = 0; i < thetas.size(); ++i)
        {
                ASSERT(thetas[i] >= 0 && thetas[i] <= 1);
                ASSERT(i <= 4);

                static_assert(ORDER >= 0 && ORDER <= 2);

                if (ORDER == 0)
                {
                        res.emplace_back(
                                std::make_unique<filters::position::Position0<N, T>>(
                                        Config<T>::POSITION_FILTER_RESET_DT, Config<T>::POSITION_FILTER_LINEAR_DT,
                                        Config<T>::POSITION_FILTER_GATE_0, thetas[i],
                                        Config<T>::POSITION_FILTER_VARIANCE_0),
                                view::Filter<N, T>(name(thetas[i]), color::RGB8(160 - 40 * i, 100, 200)));
                }

                if (ORDER == 1)
                {
                        res.emplace_back(
                                std::make_unique<filters::position::Position1<N, T>>(
                                        Config<T>::POSITION_FILTER_RESET_DT, Config<T>::POSITION_FILTER_LINEAR_DT,
                                        Config<T>::POSITION_FILTER_GATE_1, thetas[i],
                                        Config<T>::POSITION_FILTER_VARIANCE_1, Config<T>::POSITION_INIT),
                                view::Filter<N, T>(name(thetas[i]), color::RGB8(160 - 40 * i, 0, 200)));
                }

                if (ORDER == 2)
                {
                        res.emplace_back(
                                std::make_unique<filters::position::Position2<N, T>>(
                                        Config<T>::POSITION_FILTER_RESET_DT, Config<T>::POSITION_FILTER_LINEAR_DT,
                                        Config<T>::POSITION_FILTER_GATE_2, thetas[i],
                                        Config<T>::POSITION_FILTER_VARIANCE_2, Config<T>::POSITION_INIT),
                                view::Filter<N, T>(name(thetas[i]), color::RGB8(160 - 40 * i, 0, 0)));
                }
        }

        return res;
}

template <typename T>
std::vector<TestFilter<2, T>> create_accelerations()
{
        std::vector<TestFilter<2, T>> res;

        res.emplace_back(
                std::make_unique<filters::acceleration::AccelerationEkf<T>>(
                        Config<T>::ACCELERATION_MEASUREMENT_QUEUE_SIZE, Config<T>::ACCELERATION_FILTER_RESET_DT,
                        Config<T>::ACCELERATION_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::ACCELERATION_FILTER_GATE,
                        Config<T>::ACCELERATION_FILTER_POSITION_VARIANCE, Config<T>::ACCELERATION_FILTER_ANGLE_VARIANCE,
                        Config<T>::ACCELERATION_FILTER_ANGLE_R_VARIANCE, Config<T>::ACCELERATION_INIT),
                view::Filter<2, T>("Acceleration EKF", color::RGB8(0, 200, 0)));

        const int precision = compute_string_precision(Config<T>::ACCELERATION_FILTER_UKF_ALPHAS);

        const auto name = [&](const T alpha)
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Acceleration UKF (" << ALPHA << " " << alpha << ")";
                return oss.str();
        };

        const auto alphas = sort(std::array(Config<T>::ACCELERATION_FILTER_UKF_ALPHAS));
        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                ASSERT(alphas[i] > 0 && alphas[i] <= 1);
                ASSERT(i <= 4);
                res.emplace_back(
                        std::make_unique<filters::acceleration::AccelerationUkf<T>>(
                                Config<T>::ACCELERATION_MEASUREMENT_QUEUE_SIZE, Config<T>::ACCELERATION_FILTER_RESET_DT,
                                Config<T>::ACCELERATION_FILTER_ANGLE_ESTIMATION_VARIANCE,
                                Config<T>::ACCELERATION_FILTER_GATE, alphas[i],
                                Config<T>::ACCELERATION_FILTER_POSITION_VARIANCE,
                                Config<T>::ACCELERATION_FILTER_ANGLE_VARIANCE,
                                Config<T>::ACCELERATION_FILTER_ANGLE_R_VARIANCE, Config<T>::ACCELERATION_INIT),
                        view::Filter<2, T>(name(alphas[i]), color::RGB8(0, 160 - 40 * i, 0)));
        }

        return res;
}

template <typename T, std::size_t ORDER_P, std::size_t ORDER_A>
TestFilter<2, T> create_direction(const unsigned i, const T alpha)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 4);

        static_assert((ORDER_P == 1 && (ORDER_A == 0 || ORDER_A == 1)) || (ORDER_P == 2 && ORDER_A == 1));

        const int precision = compute_string_precision(Config<T>::DIRECTION_FILTER_UKF_ALPHAS);

        const auto name = [&]()
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Direction " << ORDER_P << '.' << ORDER_A << " (" << ALPHA << " " << alpha << ")";
                return oss.str();
        }();

        if (ORDER_P == 1 && ORDER_A == 0)
        {
                return {std::make_unique<filters::direction::Direction10<T>>(
                                Config<T>::DIRECTION_MEASUREMENT_QUEUE_SIZE, Config<T>::DIRECTION_FILTER_RESET_DT,
                                Config<T>::DIRECTION_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::DIRECTION_FILTER_GATE,
                                alpha, Config<T>::DIRECTION_FILTER_POSITION_VARIANCE_1_0,
                                Config<T>::DIRECTION_FILTER_ANGLE_VARIANCE_1_0, Config<T>::DIRECTION_INIT),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 250))};
        }

        if (ORDER_P == 1 && ORDER_A == 1)
        {
                return {std::make_unique<filters::direction::Direction11<T>>(
                                Config<T>::DIRECTION_MEASUREMENT_QUEUE_SIZE, Config<T>::DIRECTION_FILTER_RESET_DT,
                                Config<T>::DIRECTION_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::DIRECTION_FILTER_GATE,
                                alpha, Config<T>::DIRECTION_FILTER_POSITION_VARIANCE_1_1,
                                Config<T>::DIRECTION_FILTER_ANGLE_VARIANCE_1_1, Config<T>::DIRECTION_INIT),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 150))};
        }

        if (ORDER_P == 2 && ORDER_A == 1)
        {
                return {std::make_unique<filters::direction::Direction21<T>>(
                                Config<T>::DIRECTION_MEASUREMENT_QUEUE_SIZE, Config<T>::DIRECTION_FILTER_RESET_DT,
                                Config<T>::DIRECTION_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::DIRECTION_FILTER_GATE,
                                alpha, Config<T>::DIRECTION_FILTER_POSITION_VARIANCE_2_1,
                                Config<T>::DIRECTION_FILTER_ANGLE_VARIANCE_2_1, Config<T>::DIRECTION_INIT),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 50))};
        }
}

template <typename T>
std::vector<TestFilter<2, T>> create_directions()
{
        std::vector<TestFilter<2, T>> res;

        const auto alphas = sort(std::array(Config<T>::DIRECTION_FILTER_UKF_ALPHAS));

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 1, 0>(i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 1, 1>(i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 2, 1>(i, alphas[i]));
        }

        return res;
}

template <typename T, std::size_t ORDER_P>
TestFilter<2, T> create_speed(const unsigned i, const T alpha)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 2);

        static_assert(ORDER_P == 1 || ORDER_P == 2);

        const int precision = compute_string_precision(Config<T>::SPEED_FILTER_UKF_ALPHAS);

        const auto name = [&]()
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Speed " << ORDER_P << " (" << ALPHA << " " << alpha << ")";
                return oss.str();
        }();

        if (ORDER_P == 1)
        {
                return {std::make_unique<filters::speed::Speed1<2, T>>(
                                Config<T>::SPEED_MEASUREMENT_QUEUE_SIZE, Config<T>::SPEED_FILTER_RESET_DT,
                                Config<T>::SPEED_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::SPEED_FILTER_GATE, alpha,
                                Config<T>::SPEED_FILTER_POSITION_VARIANCE_1),
                        view::Filter<2, T>(name, color::RGB8(0, 200 - 40 * i, 0))};
        }

        if (ORDER_P == 2)
        {
                return {std::make_unique<filters::speed::Speed2<2, T>>(
                                Config<T>::SPEED_MEASUREMENT_QUEUE_SIZE, Config<T>::SPEED_FILTER_RESET_DT,
                                Config<T>::SPEED_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::SPEED_FILTER_GATE, alpha,
                                Config<T>::SPEED_FILTER_POSITION_VARIANCE_2, Config<T>::SPEED_INIT),
                        view::Filter<2, T>(name, color::RGB8(0, 150 - 40 * i, 0))};
        }
}

template <typename T>
std::vector<TestFilter<2, T>> create_speeds()
{
        std::vector<TestFilter<2, T>> res;

        const auto alphas = sort(std::array(Config<T>::SPEED_FILTER_UKF_ALPHAS));

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_speed<T, 1>(i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_speed<T, 2>(i, alphas[i]));
        }

        return res;
}
}

template <typename T>
Filters<T> create_filters()
{
        Filters<T> res;

        res.position_variance = create_position_variance<2, T>();

        res.positions_0 = create_positions<2, T, 0>();
        res.positions_1 = create_positions<2, T, 1>();
        res.positions_2 = create_positions<2, T, 2>();

        res.accelerations = create_accelerations<T>();
        res.directions = create_directions<T>();
        res.speeds = create_speeds<T>();

        res.position_estimation = std::make_unique<filters::estimation::PositionEstimation<2, T>>(
                static_cast<const filters::position::Position2<2, T>*>(res.positions_2.front().filter.get()));

        return res;
}

#define TEMPLATE(T) template Filters<T> create_filters();

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
