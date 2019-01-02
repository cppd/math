/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "conversion_span.h"

#include "conversion.h"

#include "com/error.h"

namespace color_conversion
{
std::vector<float> rgba_pixels_from_srgb_uint8_to_rgb_float(const Span<const std::uint_least8_t>& pixels)
{
        if (pixels.size() % 4 != 0)
        {
                error("sRGB pixel buffer size is not a multiple of 4");
        }

        std::vector<float> buffer(pixels.size());
        size_t b_i = 0;
        size_t p_i = 0;
        while (p_i < buffer.size())
        {
                buffer[b_i++] = srgb_uint8_to_rgb_float<float>(pixels[p_i++]);
                buffer[b_i++] = srgb_uint8_to_rgb_float<float>(pixels[p_i++]);
                buffer[b_i++] = srgb_uint8_to_rgb_float<float>(pixels[p_i++]);
                buffer[b_i++] = alpha_uint8_to_float<float>(pixels[p_i++]);
        }
        return buffer;
}

std::vector<std::uint_least16_t> rgba_pixels_from_srgb_uint8_to_rgb_uint16(const Span<const std::uint_least8_t>& pixels)
{
        if (pixels.size() % 4 != 0)
        {
                error("sRGB pixel buffer size is not a multiple of 4");
        }

        std::vector<std::uint_least16_t> buffer(pixels.size());
        size_t b_i = 0;
        size_t p_i = 0;
        while (p_i < buffer.size())
        {
                buffer[b_i++] = srgb_uint8_to_rgb_uint16(pixels[p_i++]);
                buffer[b_i++] = srgb_uint8_to_rgb_uint16(pixels[p_i++]);
                buffer[b_i++] = srgb_uint8_to_rgb_uint16(pixels[p_i++]);
                buffer[b_i++] = alpha_uint8_to_uint16(pixels[p_i++]);
        }
        return buffer;
}

std::vector<float> grayscale_pixels_from_srgb_uint8_to_rgb_float(const Span<const std::uint_least8_t>& pixels)
{
        std::vector<float> buffer(pixels.size());
        for (size_t i = 0; i < buffer.size(); ++i)
        {
                buffer[i] = srgb_uint8_to_rgb_float<float>(pixels[i]);
        }
        return buffer;
}

std::vector<std::uint_least16_t> grayscale_pixels_from_srgb_uint8_to_rgb_uint16(const Span<const std::uint_least8_t>& pixels)
{
        std::vector<std::uint_least16_t> buffer(pixels.size());
        for (size_t i = 0; i < buffer.size(); ++i)
        {
                buffer[i] = srgb_uint8_to_rgb_uint16(pixels[i]);
        }
        return buffer;
}
}
