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

namespace color_conversion
{
template <typename T>
T srgb_uint8_to_rgb_float(unsigned char c);

unsigned char srgb_uint8_to_rgb_uint8(unsigned char c);

template <typename T>
T rgb_float_to_srgb_float(T c);

template <typename T>
unsigned char rgb_float_to_srgb_uint8(T c);

template <typename T>
T rgb_float_to_rgb_luminance(T red, T green, T blue);
}
