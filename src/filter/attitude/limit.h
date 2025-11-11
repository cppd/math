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

#pragma once

#include <src/com/exponent.h>
#include <src/numerical/vector.h>

namespace ns::filter::attitude
{
template <typename T>
[[nodiscard]] bool acc_suitable(const T acc)
{
        constexpr T ACCELERATION_MIN = 9.0; // m/(s*s)
        constexpr T ACCELERATION_MAX = 10.6; // m/(s*s)

        return acc >= ACCELERATION_MIN && acc <= ACCELERATION_MAX;
}

template <typename T>
[[nodiscard]] bool acc_suitable(const numerical::Vector<3, T>& acc)
{
        constexpr T ACCELERATION_MIN = 9.0; // m/(s*s)
        constexpr T ACCELERATION_MAX = 10.6; // m/(s*s)

        constexpr T A_MIN_2 = square(ACCELERATION_MIN);
        constexpr T A_MAX_2 = square(ACCELERATION_MAX);

        const T n2 = acc.norm_squared();

        return n2 >= A_MIN_2 && n2 <= A_MAX_2;
}

template <typename T>
[[nodiscard]] bool mag_suitable(const T mag)
{
        constexpr T MAGNETIC_FIELD_MIN = 10; // uT
        constexpr T MAGNETIC_FIELD_MAX = 90; // uT

        return mag >= MAGNETIC_FIELD_MIN && mag <= MAGNETIC_FIELD_MAX;
}
}
