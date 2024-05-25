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

#pragma once

#include "init.h"

#include <src/filter/core/update_info.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>

namespace ns::filter::filters::acceleration
{
template <typename T>
class Filter0
{
public:
        virtual ~Filter0() = default;

        virtual void reset(
                const numerical::Vector<4, T>& position_velocity,
                const numerical::Matrix<4, 4, T>& position_velocity_p,
                const Init<T>& init) = 0;

        virtual void predict(
                T dt,
                const NoiseModel<T>& position_noise_model,
                const NoiseModel<T>& angle_noise_model,
                const NoiseModel<T>& angle_r_noise_model,
                T fading_memory_alpha) = 0;

        [[nodiscard]] virtual core::UpdateInfo<2, T> update_position(
                const Measurement<2, T>& position,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<3, T> update_position_speed(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<6, T> update_position_speed_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<4, T> update_position_speed_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<5, T> update_position_speed_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<5, T> update_position_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<3, T> update_position_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<4, T> update_position_acceleration(
                const Measurement<2, T>& position,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<4, T> update_speed_direction_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<2, T> update_speed_direction(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<3, T> update_direction_acceleration(
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<2, T> update_acceleration(
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<1, T> update_direction(
                const Measurement<1, T>& direction,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<1, T> update_speed(
                const Measurement<1, T>& speed,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual core::UpdateInfo<3, T> update_speed_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual numerical::Vector<2, T> position() const = 0;
        [[nodiscard]] virtual numerical::Matrix<2, 2, T> position_p() const = 0;

        [[nodiscard]] virtual T speed() const = 0;
        [[nodiscard]] virtual T speed_p() const = 0;

        [[nodiscard]] virtual T angle() const = 0;
        [[nodiscard]] virtual T angle_p() const = 0;

        [[nodiscard]] virtual T angle_r() const = 0;
        [[nodiscard]] virtual T angle_r_p() const = 0;
};

template <typename T>
[[nodiscard]] std::unique_ptr<Filter0<T>> create_filter_0(T sigma_points_alpha);
}
