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

#include "measurement.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <string>

namespace ns::filter::test
{
template <typename T>
class PositionFilter
{
public:
        virtual ~PositionFilter() = default;

        virtual void reset(const Measurement<2, T>& position) = 0;

        virtual void predict(T dt) = 0;
        virtual void update(const Measurement<2, T>& position) = 0;

        [[nodiscard]] virtual Vector<6, T> position_velocity_acceleration() const = 0;
        [[nodiscard]] virtual Matrix<6, 6, T> position_velocity_acceleration_p() const = 0;

        [[nodiscard]] virtual Vector<2, T> position() const = 0;
        [[nodiscard]] virtual Matrix<2, 2, T> position_p() const = 0;

        [[nodiscard]] virtual T speed() const = 0;
        [[nodiscard]] virtual T speed_p() const = 0;

        [[nodiscard]] virtual Vector<2, T> velocity() const = 0;
        [[nodiscard]] virtual Matrix<2, 2, T> velocity_p() const = 0;

        [[nodiscard]] virtual T angle() const = 0;
        [[nodiscard]] virtual T angle_p() const = 0;

        [[nodiscard]] virtual std::string check_string() const = 0;
};
}
