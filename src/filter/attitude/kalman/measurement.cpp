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

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/filter/attitude/limit.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
std::optional<MagMeasurement<T>> mag_measurement(
        const numerical::Vector<3, T>& z_unit,
        const numerical::Vector<3, T>& m_unit,
        const T variance)
{
        ASSERT(z_unit.is_unit());
        ASSERT(m_unit.is_unit());

        const numerical::Vector<3, T> x = cross(m_unit, z_unit);

        const T inclination_cos_2 = x.norm_squared();

        if (!(inclination_cos_2 > square(MAG_INCLINATION_MIN_COS<T>)))
        {
                return std::nullopt;
        }

        return MagMeasurement{
                .y = cross(z_unit, x).normalized(),
                .variance = variance / inclination_cos_2,
        };
}

#define TEMPLATE(T)                                                \
        template std::optional<MagMeasurement<T>> mag_measurement( \
                const numerical::Vector<3, T>&, const numerical::Vector<3, T>&, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
