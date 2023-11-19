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

namespace ns::filter::filters::speed
{
template <std::size_t N, typename T>
class Filter1
{
public:
        virtual ~Filter1() = default;

        virtual void reset(
                const Vector<N, T>& position,
                const Vector<N, T>& position_variance,
                const Vector<N, T>& velocity,
                const Vector<N, T>& velocity_variance) = 0;

        virtual void reset(
                const Vector<2 * N, T>& position_velocity,
                const Matrix<2 * N, 2 * N, T>& position_velocity_p) = 0;

        virtual void predict(T dt) = 0;

        virtual void update_position(const Measurement<N, T>& position, std::optional<T> gate) = 0;

        virtual void update_position_speed(
                const Measurement<N, T>& position,
                const Measurement<1, T>& speed,
                std::optional<T> gate) = 0;

        virtual void update_speed(const Measurement<1, T>& speed, std::optional<T> gate) = 0;

        [[nodiscard]] virtual Vector<N, T> position() const = 0;
        [[nodiscard]] virtual Matrix<N, N, T> position_p() const = 0;

        [[nodiscard]] virtual T speed() const = 0;
        [[nodiscard]] virtual T speed_p() const = 0;
};

template <std::size_t N, typename T>
std::unique_ptr<Filter1<N, T>> create_filter_1(T sigma_points_alpha, T position_variance);
}