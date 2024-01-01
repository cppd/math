/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "conversion.h"

#include "conv_from_bytes.h"
#include "conv_to_bytes.h"
#include "format.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>
#include <vector>

namespace ns::image
{
namespace
{
static_assert(sizeof(float) == sizeof(std::uint32_t));

[[noreturn]] void conversion_error(const ColorFormat from_format, const ColorFormat to_format)
{
        error("Conversion between " + format_to_string(from_format) + " and " + format_to_string(to_format)
              + " is not supported");
}

void check_equal_component_count(const ColorFormat from_format, const ColorFormat to_format)
{
        if (format_component_count(from_format) != format_component_count(to_format))
        {
                conversion_error(from_format, to_format);
        }
}

void check_component_count_alpha(const ColorFormat from_format, const ColorFormat to_format)
{
        if (!((format_component_count(from_format) == 4 || format_component_count(from_format) == 3)
              && (format_component_count(to_format) == 4 || format_component_count(to_format) == 3)))
        {
                conversion_error(from_format, to_format);
        }
}

void conv_src_to_floats(
        const ColorFormat from_format,
        const std::span<const std::byte>& from,
        const ColorFormat to_format,
        std::vector<float>* const pixels)
{
        switch (from_format)
        {
        case ColorFormat::R8_SRGB:
                check_equal_component_count(from_format, to_format);
                conv::r8_srgb_to_r32(from, pixels);
                return;
        case ColorFormat::R8G8B8_SRGB:
                check_component_count_alpha(from_format, to_format);
                conv::r8g8b8_srgb_to_r32g32b32(from, pixels);
                return;
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                check_component_count_alpha(from_format, to_format);
                conv::r8g8b8a8_srgb_to_r32g32b32a32(from, pixels);
                return;
        case ColorFormat::R16:
                check_equal_component_count(from_format, to_format);
                conv::r16_to_r32(from, pixels);
                return;
        case ColorFormat::R16G16B16:
                check_component_count_alpha(from_format, to_format);
                conv::r16g16b16_to_r32g32b32(from, pixels);
                return;
        case ColorFormat::R16G16B16_SRGB:
                check_component_count_alpha(from_format, to_format);
                conv::r16g16b16_srgb_to_r32g32b32(from, pixels);
                return;
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                check_component_count_alpha(from_format, to_format);
                conv::r16g16b16a16_to_r32g32b32a32(from, pixels);
                return;
        case ColorFormat::R16G16B16A16_SRGB:
                check_component_count_alpha(from_format, to_format);
                conv::r16g16b16a16_srgb_to_r32g32b32a32(from, pixels);
                return;
        case ColorFormat::R32:
                check_equal_component_count(from_format, to_format);
                conv::copy(from, pixels);
                return;
        case ColorFormat::R32G32B32:
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                check_component_count_alpha(from_format, to_format);
                conv::copy(from, pixels);
                return;
        }
        unknown_color_format_error(from_format);
}

void conv_floats_1_to_dst(
        const ColorFormat from_format,
        const std::vector<float>& pixels,
        const ColorFormat to_format,
        const std::span<std::byte>& to)
{
        ASSERT(format_component_count(from_format) == 1);
        switch (to_format)
        {
        case ColorFormat::R8_SRGB:
                conv::r32_to_r8_srgb(pixels, to);
                return;
        case ColorFormat::R16:
                conv::r32_to_r16(pixels, to);
                return;
        case ColorFormat::R32:
                conv::copy(pixels, to);
                return;
        case ColorFormat::R8G8B8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R32G32B32:
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                conversion_error(from_format, to_format);
        }
        unknown_color_format_error(to_format);
}

void conv_floats_3_to_dst(
        const ColorFormat from_format,
        const std::vector<float>& pixels,
        const ColorFormat to_format,
        const std::span<std::byte>& to)
{
        ASSERT(format_component_count(from_format) == 3);
        switch (to_format)
        {
        case ColorFormat::R8_SRGB:
        case ColorFormat::R16:
        case ColorFormat::R32:
                conversion_error(from_format, to_format);
        case ColorFormat::R8G8B8_SRGB:
                conv::r32g32b32_to_r8g8b8_srgb(pixels, to);
                return;
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                conv::r32g32b32_to_r8g8b8a8_srgb(pixels, to);
                return;
        case ColorFormat::R16G16B16:
                conv::r32g32b32_to_r16g16b16(pixels, to);
                return;
        case ColorFormat::R16G16B16_SRGB:
                conv::r32g32b32_to_r16g16b16_srgb(pixels, to);
                return;
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                conv::r32g32b32_to_r16g16b16a16(pixels, to);
                return;
        case ColorFormat::R16G16B16A16_SRGB:
                conv::r32g32b32_to_r16g16b16a16_srgb(pixels, to);
                return;
        case ColorFormat::R32G32B32:
                conv::copy(pixels, to);
                return;
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                conv::r32g32b32_to_r32g32b32a32(pixels, to);
                return;
        }
        unknown_color_format_error(to_format);
}

void conv_floats_4_to_dst(
        const ColorFormat from_format,
        const std::vector<float>& pixels,
        const ColorFormat to_format,
        const std::span<std::byte>& to)
{
        ASSERT(format_component_count(from_format) == 4);
        switch (to_format)
        {
        case ColorFormat::R8_SRGB:
        case ColorFormat::R16:
        case ColorFormat::R32:
                conversion_error(from_format, to_format);
        case ColorFormat::R8G8B8_SRGB:
                conv::r32g32b32a32_to_r8g8b8_srgb(pixels, to);
                return;
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                conv::r32g32b32a32_to_r8g8b8a8_srgb(pixels, to);
                return;
        case ColorFormat::R16G16B16:
                conv::r32g32b32a32_to_r16g16b16(pixels, to);
                return;
        case ColorFormat::R16G16B16_SRGB:
                conv::r32g32b32a32_to_r16g16b16_srgb(pixels, to);
                return;
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                conv::r32g32b32a32_to_r16g16b16a16(pixels, to);
                return;
        case ColorFormat::R16G16B16A16_SRGB:
                conv::r32g32b32a32_to_r16g16b16a16_srgb(pixels, to);
                return;
        case ColorFormat::R32G32B32:
                conv::r32g32b32a32_to_r32g32b32(pixels, to);
                return;
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                conv::copy(pixels, to);
                return;
        }
        unknown_color_format_error(to_format);
}

void conv_floats_to_dst(
        const ColorFormat from_format,
        const std::vector<float>& pixels,
        const ColorFormat to_format,
        const std::span<std::byte>& to)
{
        switch (format_component_count(from_format))
        {
        case 1:
                conv_floats_1_to_dst(from_format, pixels, to_format, to);
                return;
        case 3:
                conv_floats_3_to_dst(from_format, pixels, to_format, to);
                return;
        case 4:
                conv_floats_4_to_dst(from_format, pixels, to_format, to);
                return;
        }
        conversion_error(from_format, to_format);
}

void undo_alpha_multiplication(std::vector<float>* const floats)
{
        ASSERT((floats->size() % 4) == 0);

        for (std::size_t i = 0; i < floats->size(); i += 4)
        {
                const float alpha = (*floats)[i + 3];
                if (alpha != 0)
                {
                        // no clamp to [0, 1]
                        (*floats)[i + 0] /= alpha;
                        (*floats)[i + 1] /= alpha;
                        (*floats)[i + 2] /= alpha;
                }
                else
                {
                        (*floats)[i + 0] = 0;
                        (*floats)[i + 1] = 0;
                        (*floats)[i + 2] = 0;
                }
        }
}

void multiply_alpha(std::vector<float>* const floats)
{
        ASSERT((floats->size() % 4) == 0);

        for (std::size_t i = 0; i < floats->size(); i += 4)
        {
                const float alpha = (*floats)[i + 3];
                (*floats)[i + 0] *= alpha;
                (*floats)[i + 1] *= alpha;
                (*floats)[i + 2] *= alpha;
        }
}

void alpha_premultiplication(
        const ColorFormat from_format,
        const ColorFormat to_format,
        std::vector<float>* const pixels)
{
        if (is_premultiplied(from_format) && !is_premultiplied(to_format))
        {
                undo_alpha_multiplication(pixels);
        }
        else if (!is_premultiplied(from_format) && is_premultiplied(to_format))
        {
                if (format_component_count(from_format) == 4)
                {
                        multiply_alpha(pixels);
                }
        }
}

void conversion(
        const ColorFormat from_format,
        const std::span<const std::byte>& from,
        const ColorFormat to_format,
        const std::span<std::byte>& to)
{
        std::vector<float> pixels;

        conv_src_to_floats(from_format, from, to_format, &pixels);

        alpha_premultiplication(from_format, to_format, &pixels);

        conv_floats_to_dst(from_format, pixels, to_format, to);
}
}

void format_conversion(
        const ColorFormat from_format,
        const std::span<const std::byte> from,
        const ColorFormat to_format,
        const std::span<std::byte> to)
{
        ASSERT(from.data());
        ASSERT(to.data());

        const std::lldiv_t pixel_count = std::lldiv(from.size(), format_pixel_size_in_bytes(from_format));

        if (pixel_count.rem != 0)
        {
                error("Error input color data size " + to_string(from.size()) + " for color format "
                      + format_to_string(from_format));
        }

        if (pixel_count.quot * format_pixel_size_in_bytes(to_format) != static_cast<long long>(to.size()))
        {
                error("Error output color data size " + to_string(to.size()) + " for color format "
                      + format_to_string(to_format));
        }

        if (pixel_count.quot == 0)
        {
                return;
        }

        if (from_format == to_format)
        {
                ASSERT(from.size() == to.size());
                std::memcpy(to.data(), from.data(), from.size());
                return;
        }

        conversion(from_format, from, to_format, to);
}

void format_conversion(
        const ColorFormat from_format,
        const std::span<const std::byte> from,
        const ColorFormat to_format,
        std::vector<std::byte>* const to)
{
        ASSERT(from.data());
        ASSERT(to);

        const std::lldiv_t pixel_count = std::lldiv(from.size(), format_pixel_size_in_bytes(from_format));

        if (pixel_count.rem != 0)
        {
                error("Error input color data size " + to_string(from.size()) + " for color format "
                      + format_to_string(from_format));
        }

        to->resize(pixel_count.quot * format_pixel_size_in_bytes(to_format));

        if (pixel_count.quot == 0)
        {
                return;
        }

        if (from_format == to_format)
        {
                ASSERT(from.size() == std::span(*to).size_bytes());
                std::memcpy(to->data(), from.data(), from.size());
                return;
        }

        conversion(from_format, from, to_format, std::span(to->data(), to->size()));
}
}
