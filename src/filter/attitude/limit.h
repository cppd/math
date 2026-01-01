/*
Copyright (C) 2017-2026 Topological Manifold

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

namespace ns::filter::attitude
{
template <typename T>
inline constexpr T MAG_INCLINATION_MIN_COS{0.173648}; // 80 degrees

template <typename T>
[[nodiscard]] bool acc_suitable(const T acc)
{
        constexpr T ACCELERATION_MIN = 9.0; // m/(s*s)
        constexpr T ACCELERATION_MAX = 10.6; // m/(s*s)

        return acc >= ACCELERATION_MIN && acc <= ACCELERATION_MAX;
}

template <typename T>
[[nodiscard]] bool mag_suitable(const T mag)
{
        constexpr T MAGNETIC_FIELD_MIN = 10; // uT
        constexpr T MAGNETIC_FIELD_MAX = 90; // uT

        return mag >= MAGNETIC_FIELD_MIN && mag <= MAGNETIC_FIELD_MAX;
}
}
