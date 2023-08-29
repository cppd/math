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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>
#include <string>

namespace ns::filter::test
{
template <std::size_t N, typename T>
struct PositionFilterUpdate final
{
        Vector<N, T> residual;
        bool gate;
        T normalized_innovation_squared;
};

template <std::size_t N, typename T>
class PositionFilter
{
public:
        virtual ~PositionFilter() = default;

        virtual void reset(const Vector<N, T>& position, const Vector<N, T>& variance) = 0;

        virtual void predict(T dt) = 0;

        [[nodiscard]] virtual PositionFilterUpdate<N, T> update(
                const Vector<N, T>& position,
                const Vector<N, T>& variance,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual Vector<N, T> position() const = 0;
        [[nodiscard]] virtual Matrix<N, N, T> position_p() const = 0;

        [[nodiscard]] virtual bool has_speed() const = 0;
        [[nodiscard]] virtual T speed() const = 0;
        [[nodiscard]] virtual T speed_p() const = 0;

        [[nodiscard]] virtual bool has_velocity() const = 0;
        [[nodiscard]] virtual Vector<N, T> velocity() const = 0;
        [[nodiscard]] virtual Matrix<N, N, T> velocity_p() const = 0;

        [[nodiscard]] virtual bool has_position_velocity_acceleration() const = 0;
        [[nodiscard]] virtual Vector<3 * N, T> position_velocity_acceleration() const = 0;
        [[nodiscard]] virtual Matrix<3 * N, 3 * N, T> position_velocity_acceleration_p() const = 0;
};
}
