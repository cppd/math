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

#include "com/span.h"

#include <cstdint>
#include <vector>

namespace color_conversion
{
std::vector<float> rgba_pixels_from_srgb_uint8_to_rgb_float(const Span<const std::uint_least8_t>& pixels);
std::vector<std::uint_least16_t> rgba_pixels_from_srgb_uint8_to_rgb_uint16(
        const Span<const std::uint_least8_t>& pixels);

std::vector<float> grayscale_pixels_from_srgb_uint8_to_rgb_float(const Span<const std::uint_least8_t>& pixels);
std::vector<std::uint_least16_t> grayscale_pixels_from_srgb_uint8_to_rgb_uint16(
        const Span<const std::uint_least8_t>& pixels);
}
