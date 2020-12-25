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

#include "grayscale.h"

#include <src/color/conversion.h>
#include <src/com/error.h>
#include <src/com/print.h>

namespace ns::image
{
namespace
{
uint8_t rgb_to_grayscale(const std::array<uint8_t, 3>& rgb)
{
        float r = color::srgb_uint8_to_linear_float(rgb[0]);
        float g = color::srgb_uint8_to_linear_float(rgb[1]);
        float b = color::srgb_uint8_to_linear_float(rgb[2]);
        float grayscale = color::linear_float_to_linear_luminance(r, g, b);
        return color::linear_float_to_srgb_uint8(grayscale);
}

uint16_t rgb_to_grayscale(const std::array<uint16_t, 3>& rgb)
{
        float r = color::linear_uint16_to_linear_float(rgb[0]);
        float g = color::linear_uint16_to_linear_float(rgb[1]);
        float b = color::linear_uint16_to_linear_float(rgb[2]);
        float grayscale = color::linear_float_to_linear_luminance(r, g, b);
        return color::linear_float_to_linear_uint16(grayscale);
}

float rgb_to_grayscale(const std::array<float, 3>& rgb)
{
        return color::linear_float_to_linear_luminance(rgb[0], rgb[1], rgb[2]);
}

template <typename T>
void make_grayscale(ColorFormat color_format, const std::span<std::byte>& bytes)
{
        int component_count = format_component_count(color_format);
        if (component_count < 3)
        {
                error("Color component count " + to_string(bytes.size())
                      + " must be greater than or equal to 3 for grayscaling, format "
                      + format_to_string(color_format));
        }

        const size_t pixel_size = component_count * sizeof(T);
        ASSERT(pixel_size == format_pixel_size_in_bytes(color_format));
        if (bytes.size() % pixel_size != 0)
        {
                error("Error color byte count " + to_string(bytes.size()) + " for grayscaling, format "
                      + format_to_string(color_format));
        }

        std::byte* ptr = bytes.data();
        const std::byte* end = ptr + bytes.size();
        while (ptr != end)
        {
                std::array<T, 3> rgb;
                std::memcpy(rgb.data(), ptr, 3 * sizeof(T));
                T grayscale = rgb_to_grayscale(rgb);
                rgb[0] = grayscale;
                rgb[1] = grayscale;
                rgb[2] = grayscale;
                std::memcpy(ptr, rgb.data(), 3 * sizeof(T));
                ptr += pixel_size;
        }
}

template <typename T>
void convert_to_r_component_format(
        ColorFormat color_format,
        const std::span<const std::byte>& bytes_color,
        const std::span<std::byte>& bytes_r)
{
        int component_count = format_component_count(color_format);
        if (component_count < 3)
        {
                error("Color component count " + to_string(bytes_color.size())
                      + " must be greater than or equal to 3 for converting to R component format, format "
                      + format_to_string(color_format));
        }

        const size_t src_pixel_size = component_count * sizeof(T);
        ASSERT(src_pixel_size == format_pixel_size_in_bytes(color_format));
        if (bytes_color.size() % src_pixel_size != 0)
        {
                error("Error color byte count " + to_string(bytes_color.size())
                      + " for converting to R component format, format " + format_to_string(color_format));
        }

        const size_t dst_pixel_size = sizeof(T);
        if (bytes_r.size() % dst_pixel_size != 0)
        {
                error("Error R byte count " + to_string(bytes_r.size())
                      + " for converting to R component format, format " + format_to_string(color_format));
        }

        const std::byte* src = bytes_color.data();
        const std::byte* end = src + bytes_color.size();
        std::byte* dst = bytes_r.data();
        while (src != end)
        {
                T r;
                std::memcpy(&r, src, sizeof(T));
                std::memcpy(dst, &r, sizeof(T));
                src += src_pixel_size;
                dst += dst_pixel_size;
        }
}

template <typename T>
std::vector<std::byte> convert_to_r_component_format(ColorFormat color_format, const std::span<const std::byte>& bytes)
{
        std::vector<std::byte> bytes_r;
        bytes_r.resize(bytes.size() / format_component_count(color_format));
        convert_to_r_component_format<T>(color_format, bytes, bytes_r);
        return bytes_r;
}
}

void make_grayscale(ColorFormat color_format, const std::span<std::byte>& bytes)
{
        switch (color_format)
        {
        case ColorFormat::R8_SRGB:
        case ColorFormat::R16:
        case ColorFormat::R32:
                error("Unsupported image format " + format_to_string(color_format)
                      + " for converting image to grayscale");
        case ColorFormat::R8G8B8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB:
                make_grayscale<uint8_t>(color_format, bytes);
                return;
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16A16:
                make_grayscale<uint16_t>(color_format, bytes);
                return;
        case ColorFormat::R32G32B32:
        case ColorFormat::R32G32B32A32:
                make_grayscale<float>(color_format, bytes);
                return;
        }
        unknown_color_format_error(color_format);
}

std::vector<std::byte> convert_to_r_component_format(ColorFormat color_format, const std::span<const std::byte>& bytes)
{
        switch (color_format)
        {
        case ColorFormat::R8_SRGB:
        case ColorFormat::R16:
        case ColorFormat::R32:
                error("Unsupported image format " + format_to_string(color_format)
                      + " for converting to R component format");
        case ColorFormat::R8G8B8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB:
                return convert_to_r_component_format<uint8_t>(color_format, bytes);
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16A16:
                return convert_to_r_component_format<uint16_t>(color_format, bytes);
        case ColorFormat::R32G32B32:
        case ColorFormat::R32G32B32A32:
                return convert_to_r_component_format<float>(color_format, bytes);
        }
        unknown_color_format_error(color_format);
}
}
