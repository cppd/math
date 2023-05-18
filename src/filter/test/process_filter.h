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

namespace ns::filter::test
{
template <typename T>
class ProcessFilter
{
public:
        virtual ~ProcessFilter() = default;

        virtual void predict() = 0;

        virtual void update_position(const Vector<2, T>& position, T position_variance) = 0;

        virtual void update_position_speed_direction_acceleration(
                const Vector<2, T>& position,
                T speed,
                T direction,
                const Vector<2, T>& acceleration,
                T position_variance,
                T speed_variance,
                T direction_variance,
                T acceleration_variance) = 0;

        virtual void update_position_direction_acceleration(
                const Vector<2, T>& position,
                T direction,
                const Vector<2, T>& acceleration,
                T position_variance,
                T direction_variance,
                T acceleration_variance) = 0;

        virtual void update_acceleration(const Vector<2, T>& acceleration, T acceleration_variance) = 0;

        [[nodiscard]] virtual Vector<2, T> position() const = 0;
        [[nodiscard]] virtual Matrix<2, 2, T> position_p() const = 0;

        [[nodiscard]] virtual T speed() const = 0;
        [[nodiscard]] virtual T angle() const = 0;
        [[nodiscard]] virtual T angle_speed() const = 0;
        [[nodiscard]] virtual T angle_p() const = 0;
        [[nodiscard]] virtual T angle_r() const = 0;
        [[nodiscard]] virtual T angle_r_p() const = 0;
};
}
