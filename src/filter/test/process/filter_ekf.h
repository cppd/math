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

#pragma once

#include "../measurement.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>

namespace ns::filter::test::process
{
template <typename T>
class FilterEkf
{
public:
        virtual ~FilterEkf() = default;

        virtual void reset(
                const Vector<6, T>& position_velocity_acceleration,
                const Matrix<6, 6, T>& position_velocity_acceleration_p,
                T angle,
                T angle_variance) = 0;

        virtual void reset(
                const Vector<4, T>& position_velocity,
                const Matrix<4, 4, T>& position_velocity_p,
                T angle,
                T angle_variance) = 0;

        virtual void predict(T dt) = 0;

        virtual void update_position(const Measurement<2, T>& position, std::optional<T> gate) = 0;

        virtual void update_position_speed(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                std::optional<T> gate) = 0;

        virtual void update_position_speed_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        virtual void update_position_speed_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                std::optional<T> gate) = 0;

        virtual void update_position_speed_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        virtual void update_position_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        virtual void update_position_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                std::optional<T> gate) = 0;

        virtual void update_position_acceleration(
                const Measurement<2, T>& position,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        virtual void update_speed_direction_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        virtual void update_speed_direction(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                std::optional<T> gate) = 0;

        virtual void update_direction_acceleration(
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        virtual void update_acceleration(const Measurement<2, T>& acceleration, std::optional<T> gate) = 0;

        virtual void update_direction(const Measurement<1, T>& direction, std::optional<T> gate) = 0;

        virtual void update_speed(const Measurement<1, T>& speed, std::optional<T> gate) = 0;

        virtual void update_speed_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual Vector<2, T> position() const = 0;
        [[nodiscard]] virtual Matrix<2, 2, T> position_p() const = 0;
        [[nodiscard]] virtual T speed() const = 0;
        [[nodiscard]] virtual T speed_p() const = 0;
        [[nodiscard]] virtual T angle() const = 0;
        [[nodiscard]] virtual T angle_p() const = 0;
        [[nodiscard]] virtual T angle_speed() const = 0;
        [[nodiscard]] virtual T angle_speed_p() const = 0;
        [[nodiscard]] virtual T angle_r() const = 0;
        [[nodiscard]] virtual T angle_r_p() const = 0;
};

template <typename T>
std::unique_ptr<FilterEkf<T>> create_filter_ekf(T position_variance, T angle_variance, T angle_r_variance);
}
