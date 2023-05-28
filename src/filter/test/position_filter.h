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

#include "../ekf.h"

#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter::test
{
template <typename T>
struct PositionFilterInit final
{
        static constexpr Vector<2, T> VELOCITY{0};
        static constexpr Vector<2, T> ACCELERAION{0};
        static constexpr T SPEED_VARIANCE = square(30.0);
        static constexpr T ACCELERATION_VARIANCE = square(10.0);

        Vector<2, T> position;
        T position_variance;
};

template <typename T>
class PositionFilter final
{
        Ekf<6, T> filter_;
        T process_variance_;

public:
        PositionFilter(const PositionFilterInit<T>& init, T process_variance);

        void predict(T dt);
        void update(const Vector<2, T>& position, T measurement_variance);

        [[nodiscard]] Vector<2, T> position() const;
        [[nodiscard]] Matrix<2, 2, T> position_p() const;

        [[nodiscard]] T speed() const;
        [[nodiscard]] Vector<2, T> velocity() const;

        [[nodiscard]] T angle() const;
        [[nodiscard]] T angle_p() const;
};
}
