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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter::test
{
template <typename T>
class PositionFilter final
{
        Ekf<6, T> filter_;
        T process_variance_;

public:
        PositionFilter(T process_variance, const Vector<6, T>& x, const Matrix<6, 6, T>& p);

        void predict(T dt);
        void update(const Vector<2, T>& position, T measurement_variance);

        [[nodiscard]] Vector<2, T> position() const;
        [[nodiscard]] Matrix<2, 2, T> position_p() const;

        struct Angle final
        {
                T angle;
                T variance;
        };

        [[nodiscard]] Angle velocity_angle() const;
};
}
