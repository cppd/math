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

#include "../sigma_points.h"
#include "../ukf.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter::test
{
template <typename T>
class ProcessFilterUkf final
{
        Ukf<9, T, SigmaPoints> filter_;
        const T dt_;
        Matrix<9, 9, T> q_;

public:
        ProcessFilterUkf(
                T dt,
                T position_variance,
                T angle_variance,
                T angle_r_variance,
                const Vector<9, T>& x,
                const Matrix<9, 9, T>& p);

        void predict();

        void update_position(const Vector<2, T>& position, T position_variance);

        void update_position_speed_direction_acceleration(
                const Vector<2, T>& position,
                T speed,
                T direction,
                const Vector<2, T>& acceleration,
                T position_variance,
                T speed_variance,
                T direction_variance,
                T acceleration_variance);

        void update_position_direction_acceleration(
                const Vector<2, T>& position,
                T direction,
                const Vector<2, T>& acceleration,
                T position_variance,
                T direction_variance,
                T acceleration_variance);

        void update_acceleration(const Vector<2, T>& acceleration, T acceleration_variance);

        [[nodiscard]] Vector<2, T> position() const;
        [[nodiscard]] Matrix<2, 2, T> position_p() const;

        [[nodiscard]] T speed() const;
        [[nodiscard]] T angle() const;
        [[nodiscard]] T angle_speed() const;
        [[nodiscard]] T angle_p() const;
        [[nodiscard]] T angle_r() const;
        [[nodiscard]] T angle_r_p() const;
};

}
