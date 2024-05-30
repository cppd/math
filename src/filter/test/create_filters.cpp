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

#include "view/write.h"

#include <src/color/rgb8.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/sort.h>
#include <src/filter/filters/acceleration/acceleration.h>
#include <src/filter/filters/acceleration/init.h>
#include <src/filter/filters/direction/direction.h>
#include <src/filter/filters/direction/init.h>
#include <src/filter/filters/estimation/position_estimation.h>
#include <src/filter/filters/estimation/position_variance.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/filters/position/init.h>
#include <src/filter/filters/position/position.h>
#include <src/filter/filters/speed/init.h>
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

template <typename T>
using DiscreteNoise = filters::DiscreteNoiseModel<T>;

template <typename T>
struct Position final
{
        static constexpr DiscreteNoise<T> NOISE_MODEL_0{.variance = square(0.5)};
        static constexpr T FADING_MEMORY_ALPHA_0 = 1;
        static constexpr std::optional<T> GATE_0{};

        static constexpr DiscreteNoise<T> NOISE_MODEL_1{.variance = square(1)};
        static constexpr T FADING_MEMORY_ALPHA_1 = 1;
        static constexpr std::optional<T> GATE_1{10};

        static constexpr DiscreteNoise<T> NOISE_MODEL_2{.variance = square(0.5)};
        static constexpr T FADING_MEMORY_ALPHA_2 = 1;
        static constexpr std::optional<T> GATE_2{5};

        static constexpr std::array THETAS = std::to_array<T>({0});
        static constexpr T RESET_DT = 10;
        static constexpr T LINEAR_DT = 2;
        static constexpr filters::position::Init<T> INIT{
                .speed = 0,
                .speed_variance = square<T>(30),
                .acceleration = 0,
                .acceleration_variance = square<T>(10)};
};

template <typename T>
struct PositionVariance final
{
        static constexpr T RESET_DT = 10;
        static constexpr DiscreteNoise<T> NOISE_MODEL_2{.variance = square(0.5)};
        static constexpr T FADING_MEMORY_ALPHA_2 = 1;
        static constexpr filters::position::Init<T> INIT{
                .speed = 0,
                .speed_variance = square<T>(30),
                .acceleration = 0,
                .acceleration_variance = square<T>(10)};
};

template <typename T>
struct Acceleration final
{
        static constexpr DiscreteNoise<T> POSITION_NOISE_MODEL{.variance = square(1.0)};
        static constexpr DiscreteNoise<T> ANGLE_NOISE_MODEL_0{.variance = square(degrees_to_radians(1.0))};
        static constexpr DiscreteNoise<T> ANGLE_R_NOISE_MODEL_0{.variance = square(degrees_to_radians(1.0))};
        static constexpr DiscreteNoise<T> ANGLE_NOISE_MODEL_1{.variance = square(degrees_to_radians(0.001))};
        static constexpr DiscreteNoise<T> ANGLE_R_NOISE_MODEL_1{.variance = square(degrees_to_radians(0.001))};
        static constexpr T ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr T FADING_MEMORY_ALPHA_0 = 1.001;
        static constexpr T FADING_MEMORY_ALPHA_1 = 1.001;
        static constexpr std::array UKF_ALPHAS = std::to_array<T>({0.1, 1.0});
        static constexpr T RESET_DT = 10;
        static constexpr std::optional<T> GATE{};
        static constexpr std::size_t MEASUREMENT_QUEUE_SIZE = 20;
        static constexpr filters::acceleration::Init<T> INIT{
                .angle = 0,
                .angle_variance = square(degrees_to_radians(100.0)),
                .acceleration = 0,
                .acceleration_variance = square(10),
                .angle_speed = 0,
                .angle_speed_variance = square(degrees_to_radians(1.0)),
                .angle_r = 0,
                .angle_r_variance = square(degrees_to_radians(50.0))};
};

template <typename T>
struct Direction final
{
        static constexpr DiscreteNoise<T> POSITION_NOISE_MODEL_1_0{.variance = square(2.0)};
        static constexpr DiscreteNoise<T> POSITION_NOISE_MODEL_1_1{.variance = square(2.0)};
        static constexpr DiscreteNoise<T> POSITION_NOISE_MODEL_2_1{.variance = square(1.0)};
        static constexpr DiscreteNoise<T> ANGLE_NOISE_MODEL_1_0{.variance = square(degrees_to_radians(0.2))};
        static constexpr DiscreteNoise<T> ANGLE_NOISE_MODEL_1_1{.variance = square(degrees_to_radians(0.001))};
        static constexpr DiscreteNoise<T> ANGLE_NOISE_MODEL_2_1{.variance = square(degrees_to_radians(0.001))};
        static constexpr T ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr T FADING_MEMORY_ALPHA_1_0 = 1.001;
        static constexpr T FADING_MEMORY_ALPHA_1_1 = 1.001;
        static constexpr T FADING_MEMORY_ALPHA_2_1 = 1;
        static constexpr std::array UKF_ALPHAS = std::to_array<T>({1.0});
        static constexpr T RESET_DT = 10;
        static constexpr std::optional<T> GATE{};
        static constexpr std::size_t MEASUREMENT_QUEUE_SIZE = 20;
        static constexpr filters::direction::Init<T> INIT{
                .angle = 0,
                .angle_variance = square(degrees_to_radians(100.0)),
                .acceleration = 0,
                .acceleration_variance = square(10),
                .angle_speed = 0,
                .angle_speed_variance = square(degrees_to_radians(1.0))};
};

template <typename T>
struct Speed final
{
        static constexpr DiscreteNoise<T> NOISE_MODEL_1{.variance = square(2.0)};
        static constexpr DiscreteNoise<T> NOISE_MODEL_2{.variance = square(2.0)};
        static constexpr T ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr T FADING_MEMORY_ALPHA_1 = 1.001;
        static constexpr T FADING_MEMORY_ALPHA_2 = 1.001;
        static constexpr std::array UKF_ALPHAS = std::to_array<T>({1.0});
        static constexpr T RESET_DT = 10;
        static constexpr std::optional<T> GATE{};
        static constexpr std::size_t MEASUREMENT_QUEUE_SIZE = 20;
        static constexpr filters::speed::Init<T> INIT{.acceleration = 0, .acceleration_variance = square(10)};
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
                PositionVariance<T>::RESET_DT, PositionVariance<T>::NOISE_MODEL_2,
                PositionVariance<T>::FADING_MEMORY_ALPHA_2, PositionVariance<T>::INIT);
}

template <std::size_t N, typename T, std::size_t ORDER>
TestFilterPosition<N, T> create_position(const unsigned i, const T theta)
{
        ASSERT(theta >= 0 && theta <= 1);
        ASSERT(i <= 4);

        const int precision = compute_string_precision(Position<T>::THETAS);

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
                                Position<T>::RESET_DT, Position<T>::LINEAR_DT, Position<T>::GATE_0, Position<T>::INIT,
                                theta, Position<T>::NOISE_MODEL_0, Position<T>::FADING_MEMORY_ALPHA_0),
                        view::Filter<N, T>(name, color::RGB8(160 - 40 * i, 100, 200))};
        case 1:
                return {filters::position::create_position_1<N, T>(
                                Position<T>::RESET_DT, Position<T>::LINEAR_DT, Position<T>::GATE_1, Position<T>::INIT,
                                theta, Position<T>::NOISE_MODEL_1, Position<T>::FADING_MEMORY_ALPHA_1),
                        view::Filter<N, T>(name, color::RGB8(160 - 40 * i, 0, 200))};
        case 2:
                return {filters::position::create_position_2<N, T>(
                                Position<T>::RESET_DT, Position<T>::LINEAR_DT, Position<T>::GATE_2, Position<T>::INIT,
                                theta, Position<T>::NOISE_MODEL_2, Position<T>::FADING_MEMORY_ALPHA_2),
                        view::Filter<N, T>(name, color::RGB8(160 - 40 * i, 0, 0))};
        default:
                ASSERT(false);
        }
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<TestFilterPosition<N, T>> create_positions()
{
        std::vector<TestFilterPosition<N, T>> res;

        const auto thetas = sort(std::array(Position<T>::THETAS));

        for (std::size_t i = 0; i < thetas.size(); ++i)
        {
                res.push_back(create_position<N, T, ORDER>(i, thetas[i]));
        }

        return res;
}

template <typename T, std::size_t ORDER>
TestFilter<2, T> create_acceleration(const unsigned i, const T alpha)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 4);

        const int precision = compute_string_precision(Acceleration<T>::UKF_ALPHAS);

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
                                Acceleration<T>::MEASUREMENT_QUEUE_SIZE, Acceleration<T>::RESET_DT,
                                Acceleration<T>::ANGLE_ESTIMATION_VARIANCE, Acceleration<T>::GATE,
                                Acceleration<T>::INIT, alpha, Acceleration<T>::POSITION_NOISE_MODEL,
                                Acceleration<T>::ANGLE_NOISE_MODEL_0, Acceleration<T>::ANGLE_R_NOISE_MODEL_0,
                                Acceleration<T>::FADING_MEMORY_ALPHA_0),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 0))};
        case 1:
                return {filters::acceleration::create_acceleration_1<T>(
                                Acceleration<T>::MEASUREMENT_QUEUE_SIZE, Acceleration<T>::RESET_DT,
                                Acceleration<T>::ANGLE_ESTIMATION_VARIANCE, Acceleration<T>::GATE,
                                Acceleration<T>::INIT, alpha, Acceleration<T>::POSITION_NOISE_MODEL,
                                Acceleration<T>::ANGLE_NOISE_MODEL_1, Acceleration<T>::ANGLE_R_NOISE_MODEL_1,
                                Acceleration<T>::FADING_MEMORY_ALPHA_1),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 0))};
        default:
                ASSERT(false);
        }
}

template <typename T>
std::vector<TestFilter<2, T>> create_accelerations()
{
        std::vector<TestFilter<2, T>> res;

        res.emplace_back(
                filters::acceleration::create_acceleration_ekf<T>(
                        Acceleration<T>::MEASUREMENT_QUEUE_SIZE, Acceleration<T>::RESET_DT,
                        Acceleration<T>::ANGLE_ESTIMATION_VARIANCE, Acceleration<T>::GATE, Acceleration<T>::INIT,
                        Acceleration<T>::POSITION_NOISE_MODEL, Acceleration<T>::ANGLE_NOISE_MODEL_1,
                        Acceleration<T>::ANGLE_R_NOISE_MODEL_1, Acceleration<T>::FADING_MEMORY_ALPHA_1),
                view::Filter<2, T>("Acceleration EKF", color::RGB8(0, 200, 0)));

        const auto alphas = sort(std::array(Direction<T>::UKF_ALPHAS));

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_acceleration<T, 0>(i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_acceleration<T, 1>(i, alphas[i]));
        }

        return res;
}

template <typename T, std::size_t ORDER>
TestFilter<2, T> create_direction(const unsigned i, const T alpha)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 4);

        const int precision = compute_string_precision(Direction<T>::UKF_ALPHAS);

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
                                Direction<T>::MEASUREMENT_QUEUE_SIZE, Direction<T>::RESET_DT,
                                Direction<T>::ANGLE_ESTIMATION_VARIANCE, Direction<T>::GATE, Direction<T>::INIT, alpha,
                                Direction<T>::POSITION_NOISE_MODEL_1_0, Direction<T>::ANGLE_NOISE_MODEL_1_0,
                                Direction<T>::FADING_MEMORY_ALPHA_1_0),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 250))};
        case 11:
                return {filters::direction::create_direction_1_1<T>(
                                Direction<T>::MEASUREMENT_QUEUE_SIZE, Direction<T>::RESET_DT,
                                Direction<T>::ANGLE_ESTIMATION_VARIANCE, Direction<T>::GATE, Direction<T>::INIT, alpha,
                                Direction<T>::POSITION_NOISE_MODEL_1_1, Direction<T>::ANGLE_NOISE_MODEL_1_1,
                                Direction<T>::FADING_MEMORY_ALPHA_1_1),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 150))};
        case 21:
                return {filters::direction::create_direction_2_1<T>(
                                Direction<T>::MEASUREMENT_QUEUE_SIZE, Direction<T>::RESET_DT,
                                Direction<T>::ANGLE_ESTIMATION_VARIANCE, Direction<T>::GATE, Direction<T>::INIT, alpha,
                                Direction<T>::POSITION_NOISE_MODEL_2_1, Direction<T>::ANGLE_NOISE_MODEL_2_1,
                                Direction<T>::FADING_MEMORY_ALPHA_2_1),
                        view::Filter<2, T>(name, color::RGB8(0, 160 - 40 * i, 50))};
        default:
                ASSERT(false);
        }
}

template <typename T>
std::vector<TestFilter<2, T>> create_directions()
{
        std::vector<TestFilter<2, T>> res;

        const auto alphas = sort(std::array(Direction<T>::UKF_ALPHAS));

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 10>(i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 11>(i, alphas[i]));
        }

        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_direction<T, 21>(i, alphas[i]));
        }

        return res;
}

template <typename T, std::size_t ORDER_P>
TestFilter<2, T> create_speed(const unsigned i, const T alpha)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 2);

        const int precision = compute_string_precision(Speed<T>::UKF_ALPHAS);

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
                                Speed<T>::MEASUREMENT_QUEUE_SIZE, Speed<T>::RESET_DT,
                                Speed<T>::ANGLE_ESTIMATION_VARIANCE, Speed<T>::GATE, Speed<T>::INIT, alpha,
                                Speed<T>::NOISE_MODEL_1, Speed<T>::FADING_MEMORY_ALPHA_1),
                        view::Filter<2, T>(name, color::RGB8(0, 200 - 40 * i, 0))};
        case 2:
                return {filters::speed::create_speed_2<2, T>(
                                Speed<T>::MEASUREMENT_QUEUE_SIZE, Speed<T>::RESET_DT,
                                Speed<T>::ANGLE_ESTIMATION_VARIANCE, Speed<T>::GATE, Speed<T>::INIT, alpha,
                                Speed<T>::NOISE_MODEL_2, Speed<T>::FADING_MEMORY_ALPHA_2),
                        view::Filter<2, T>(name, color::RGB8(0, 150 - 40 * i, 0))};
        default:
                ASSERT(false);
        }
}

template <typename T>
std::vector<TestFilter<2, T>> create_speeds()
{
        std::vector<TestFilter<2, T>> res;

        const auto alphas = sort(std::array(Speed<T>::UKF_ALPHAS));

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

        res.position_estimation =
                std::make_unique<filters::estimation::PositionEstimation<2, T>>(res.positions_2.front().filter.get());

        return res;
}

#define TEMPLATE(T) template Filters<T> create_filters();

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
