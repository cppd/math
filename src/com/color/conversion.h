/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include <cstdint>

namespace color_conversion
{
template <typename T, typename UInt8>
T srgb_uint8_to_rgb_float(UInt8 c);

template <typename UInt8>
unsigned char srgb_uint8_to_rgb_uint8(UInt8 c);

template <typename UInt8>
std::uint_least16_t srgb_uint8_to_rgb_uint16(UInt8 c);

//

template <typename T, typename UInt8>
T alpha_uint8_to_float(UInt8 c);

template <typename UInt8>
std::uint_least16_t alpha_uint8_to_uint16(UInt8 c);

//

template <typename T>
T rgb_float_to_srgb_float(T c);

template <typename T>
unsigned char rgb_float_to_srgb_uint8(T c);

//

template <typename T>
T rgb_float_to_rgb_luminance(T red, T green, T blue);
}
