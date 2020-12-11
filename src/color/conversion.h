/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <cstdint>

namespace color
{
template <typename T, typename UInt8>
T srgb_uint8_to_linear_float(UInt8 c);

template <typename T, typename UInt8>
T linear_uint8_to_linear_float(UInt8 c);

//

template <typename T>
T linear_float_to_srgb_float(T c);

//

template <typename T>
unsigned char linear_float_to_srgb_uint8(T c);

template <typename T>
unsigned char linear_float_to_linear_uint8(T c);

//

template <typename T>
T linear_float_to_linear_luminance(T red, T green, T blue);

//

template <typename T>
std::uint16_t float_to_uint16(T c)
{
        static_assert(std::is_same_v<T, float>);
        constexpr float MAX = limits<std::uint16_t>::max();
        ASSERT(c >= 0.0f && c <= 1.0f);
        return c * MAX + 0.5f;
}

template <typename T>
T uint16_to_float(T c)
{
        static_assert(std::is_same_v<T, std::uint16_t>);
        constexpr float MAX = limits<std::uint16_t>::max();
        return c / MAX;
}
}
