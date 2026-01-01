/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/conversion.h>
#include <src/com/exponent.h>
#include <src/filter/filters/acceleration/init.h>
#include <src/filter/filters/direction/init.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/filters/position/init.h>
#include <src/filter/filters/speed/init.h>

#include <array>
#include <cstddef>
#include <optional>

namespace ns::filter::test
{
template <typename T>
struct PositionConfig final
{
        filters::DiscreteNoiseModel<T> noise_model_0{.variance = square(0.5)};
        T fading_memory_alpha_0 = 1;
        std::optional<T> gate_0{};

        filters::DiscreteNoiseModel<T> noise_model_1{.variance = square(1)};
        T fading_memory_alpha_1 = 1;
        std::optional<T> gate_1{10};

        filters::DiscreteNoiseModel<T> noise_model_2{.variance = square(0.5)};
        T fading_memory_alpha_2 = 1;
        std::optional<T> gate_2{5};

        std::array<T, 1> thetas = std::to_array<T>({0});
        T reset_dt = 10;
        T linear_dt = 2;
        filters::position::Init<T> init{
                .speed = 0,
                .speed_variance = square<T>(30),
                .acceleration = 0,
                .acceleration_variance = square<T>(10)};
};

template <typename T>
struct PositionVarianceConfig final
{
        T reset_dt = 10;
        filters::DiscreteNoiseModel<T> noise_model_2{.variance = square(0.5)};
        T fading_memory_alpha_2 = 1;
        filters::position::Init<T> init{
                .speed = 0,
                .speed_variance = square<T>(30),
                .acceleration = 0,
                .acceleration_variance = square<T>(10)};
};

template <typename T>
struct AccelerationConfig final
{
        filters::DiscreteNoiseModel<T> position_noise_model{.variance = square(1.0)};
        filters::DiscreteNoiseModel<T> angle_noise_model_0{.variance = square(degrees_to_radians(1.0))};
        filters::DiscreteNoiseModel<T> angle_r_noise_model_0{.variance = square(degrees_to_radians(1.0))};
        filters::DiscreteNoiseModel<T> angle_noise_model_1{.variance = square(degrees_to_radians(0.001))};
        filters::DiscreteNoiseModel<T> angle_r_noise_model_1{.variance = square(degrees_to_radians(0.001))};
        T angle_estimation_variance = square(degrees_to_radians(20.0));
        T fading_memory_alpha_0 = 1.001;
        T fading_memory_alpha_1 = 1.001;
        std::array<T, 2> ukf_alphas = std::to_array<T>({0.1, 1.0});
        T reset_dt = 10;
        std::optional<T> gate{};
        std::size_t measurement_queue_size = 20;
        filters::acceleration::Init<T> init{
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
struct DirectionConfig final
{
        filters::DiscreteNoiseModel<T> position_noise_model_1_0{.variance = square(2.0)};
        filters::DiscreteNoiseModel<T> position_noise_model_1_1{.variance = square(2.0)};
        filters::DiscreteNoiseModel<T> position_noise_model_2_1{.variance = square(1.0)};
        filters::DiscreteNoiseModel<T> angle_noise_model_1_0{.variance = square(degrees_to_radians(0.2))};
        filters::DiscreteNoiseModel<T> angle_noise_model_1_1{.variance = square(degrees_to_radians(0.001))};
        filters::DiscreteNoiseModel<T> angle_noise_model_2_1{.variance = square(degrees_to_radians(0.001))};
        T angle_estimation_variance = square(degrees_to_radians(20.0));
        T fading_memory_alpha_1_0 = 1.001;
        T fading_memory_alpha_1_1 = 1.001;
        T fading_memory_alpha_2_1 = 1;
        std::array<T, 1> ukf_alphas = std::to_array<T>({1.0});
        T reset_dt = 10;
        std::optional<T> gate{};
        std::size_t measurement_queue_size = 20;
        filters::direction::Init<T> init{
                .angle = 0,
                .angle_variance = square(degrees_to_radians(100.0)),
                .acceleration = 0,
                .acceleration_variance = square(10),
                .angle_speed = 0,
                .angle_speed_variance = square(degrees_to_radians(1.0))};
};

template <typename T>
struct SpeedConfig final
{
        filters::DiscreteNoiseModel<T> noise_model_1{.variance = square(2.0)};
        filters::DiscreteNoiseModel<T> noise_model_2{.variance = square(2.0)};
        T angle_estimation_variance = square(degrees_to_radians(20.0));
        T fading_memory_alpha_1 = 1.001;
        T fading_memory_alpha_2 = 1.001;
        std::array<T, 1> ukf_alphas = std::to_array<T>({1.0});
        T reset_dt = 10;
        std::optional<T> gate{};
        std::size_t measurement_queue_size = 20;
        filters::speed::Init<T> init{.acceleration = 0, .acceleration_variance = square(10)};
};
}
