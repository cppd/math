/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "measurement.h"

#include "constant.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
std::optional<MagMeasurement<T>> mag_measurement(
        const numerical::Matrix<3, 3, T>& attitude,
        const numerical::Vector<3, T>& m_unit,
        const T variance)
{
        ASSERT(m_unit.is_unit());

        // attitude * Vector(0, 0, 1)
        const numerical::Vector<3, T> z_unit = attitude.column(2);
        ASSERT(z_unit.is_unit());

        const numerical::Vector<3, T> x = cross(m_unit, z_unit);

        const T sin2 = x.norm_squared();
        if (!(sin2 > square(MIN_SIN_Z_MAG<T>)))
        {
                return std::nullopt;
        }

        return MagMeasurement{
                .y = cross(z_unit, x).normalized(),
                .variance = variance / sin2,
        };
}

#define TEMPLATE(T)                                                \
        template std::optional<MagMeasurement<T>> mag_measurement( \
                const numerical::Matrix<3, 3, T>&, const numerical::Vector<3, T>&, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
