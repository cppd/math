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
class Estimation
{
protected:
        ~Estimation() = default;

public:
        [[nodiscard]] virtual bool has_angle_p() const = 0;
        [[nodiscard]] virtual T angle_p() const = 0;

        [[nodiscard]] virtual Vector<6, T> position_velocity_acceleration() const = 0;
        [[nodiscard]] virtual Matrix<6, 6, T> position_velocity_acceleration_p() const = 0;
        [[nodiscard]] virtual std::string position_description() const = 0;

        [[nodiscard]] virtual bool has_angle_difference() const = 0;
        [[nodiscard]] virtual T angle_difference() const;
        [[nodiscard]] virtual std::string angle_difference_description() const = 0;
};
}
