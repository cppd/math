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
namespace limit_implementation
{
template <typename T>
inline constexpr T ACCELERATION_MIN = 9.0; // m/(s*s)

template <typename T>
inline constexpr T ACCELERATION_MAX = 10.6; // m/(s*s)

template <typename T>
inline constexpr T MAGNETIC_FIELD_MIN = 10; // uT

template <typename T>
inline constexpr T MAGNETIC_FIELD_MAX = 90; // uT
}

template <typename T>
[[nodiscard]] bool acc_suitable(const T acc)
{
        namespace impl = limit_implementation;

        return acc >= impl::ACCELERATION_MIN<T> && acc <= impl::ACCELERATION_MAX<T>;
}

template <typename T>
[[nodiscard]] bool acc_suitable(const numerical::Vector<3, T>& acc)
{
        namespace impl = limit_implementation;

        constexpr T MIN_2 = square(impl::ACCELERATION_MIN<T>);
        constexpr T MAX_2 = square(impl::ACCELERATION_MAX<T>);

        const T n2 = acc.norm_squared();

        return n2 >= MIN_2 && n2 <= MAX_2;
}

template <typename T>
[[nodiscard]] bool mag_suitable(const T mag)
{
        namespace impl = limit_implementation;

        return mag >= impl::MAGNETIC_FIELD_MIN<T> && mag <= impl::MAGNETIC_FIELD_MAX<T>;
}

template <typename T>
[[nodiscard]] bool mag_suitable(const numerical::Vector<3, T>& mag)
{
        namespace impl = limit_implementation;

        constexpr T MIN_2 = square(impl::MAGNETIC_FIELD_MIN<T>);
        constexpr T MAX_2 = square(impl::MAGNETIC_FIELD_MAX<T>);

        const T n2 = mag.norm_squared();

        return n2 >= MIN_2 && n2 <= MAX_2;
}
}
