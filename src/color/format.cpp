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

#include "format.h"

#include "conversion.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

namespace color
{
namespace
{
static_assert(sizeof(float) == sizeof(uint32_t));

template <typename T>
std::string enum_to_string(T e)
{
        static_assert(sizeof(e) <= sizeof(long long));

        return to_string(static_cast<long long>(e));
}

[[noreturn]] void unknown_color_format_error(ColorFormat format)
{
        error_fatal("Unknown color format " + enum_to_string(format));
}

[[noreturn]] void component_count_error(ColorFormat from_format, ColorFormat to_format)
{
        error("Conversion between " + format_to_string(from_format) + " and " + format_to_string(to_format)
              + " is not supported");
}

template <typename T>
std::uint16_t float_to_uint16(T c)
{
        static_assert(std::is_same_v<T, float>);
        return c * float(limits<uint16_t>::max()) + 0.5f;
}

template <typename T>
float uint16_to_float(T c)
{
        static_assert(std::is_same_v<T, std::uint16_t>);
        return float(c) / float(limits<uint16_t>::max());
}

void conv_r8_srgb_to_r32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint8_t;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 1) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        auto to = floats->begin();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(red);
                from += sizeof(From);
        }
}

void conv_r16_to_r32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint16_t;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 1) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        auto to = floats->begin();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = uint16_to_float(red);
                from += sizeof(From);
        }
}

void conv_r8g8b8_srgb_to_r32g32b32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint8_t;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(blue);
                from += sizeof(From);
        }
}

void conv_r16g16b16_to_r32g32b32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint16_t;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = uint16_to_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = uint16_to_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = uint16_to_float(blue);
                from += sizeof(From);
        }
}

void conv_r8g8b8a8_srgb_to_r32g32b32a32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint8_t;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(blue);
                from += sizeof(From);

                From alpha;
                std::memcpy(&alpha, from, sizeof(From));
                *to++ = color_conversion::linear_uint8_to_linear_float<float>(alpha);
                from += sizeof(From);
        }
}

void conv_r8g8b8a8_srgb_to_r32g32b32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint8_t;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT((component_count % 4) == 0);
        ASSERT(bytes.size() == (component_count * sizeof(From)));

        floats->resize((component_count / 4) * 3);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color_conversion::srgb_uint8_to_linear_float<float>(blue);
                from += sizeof(From);

                from += sizeof(From);
        }
}

void conv_r16g16b16a16_to_r32g32b32a32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint16_t;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = uint16_to_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = uint16_to_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = uint16_to_float(blue);
                from += sizeof(From);

                From alpha;
                std::memcpy(&alpha, from, sizeof(From));
                *to++ = uint16_to_float(alpha);
                from += sizeof(From);
        }
}

void conv_r16g16b16a16_to_r32g32b32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint16_t;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize((component_count / 4) * 3);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = uint16_to_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = uint16_to_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = uint16_to_float(blue);
                from += sizeof(From);

                from += sizeof(From);
        }
}

void conv_r32g32b32a32_to_r32g32b32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = float;

        unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize((component_count / 4) * 3);

        const std::byte* from = bytes.data();
        const std::byte* end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                std::memcpy(to, from, 3 * sizeof(From));
                to += 3;
                from += 4 * sizeof(From);
        }
}

void conv_copy(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        unsigned component_count = bytes.size() / sizeof(float);
        ASSERT(bytes.size() == (component_count * sizeof(float)));

        floats->resize(component_count);

        ASSERT(data_size(*floats) == bytes.size());
        std::memcpy(floats->data(), bytes.data(), bytes.size());
}

void conv_r32_to_r8_srgb(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = uint8_t;

        unsigned component_count = floats.size();
        ASSERT((component_count % 1) == 0);

        ASSERT(bytes.size() == component_count * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32_to_r16(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = uint16_t;

        unsigned component_count = floats.size();
        ASSERT((component_count % 1) == 0);

        ASSERT(bytes.size() == component_count * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = float_to_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32g32b32_to_r8g8b8_srgb(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = uint8_t;

        unsigned component_count = floats.size();
        ASSERT((component_count % 3) == 0);

        ASSERT(bytes.size() == component_count * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32g32b32_to_r8g8b8a8_srgb(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = uint8_t;

        unsigned component_count = floats.size();
        ASSERT((component_count % 3) == 0);

        ASSERT(bytes.size() == (component_count / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = limits<To>::max();
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32g32b32_to_r16g16b16(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = uint16_t;

        unsigned component_count = floats.size();
        ASSERT((component_count % 3) == 0);

        ASSERT(bytes.size() == component_count * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = float_to_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = float_to_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = float_to_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32g32b32_to_r16g16b16a16(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = uint16_t;

        unsigned component_count = floats.size();
        ASSERT((component_count % 3) == 0);

        ASSERT(bytes.size() == (component_count / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = float_to_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = float_to_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = float_to_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = limits<To>::max();
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32g32b32_to_r32g32b32a32(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = float;

        unsigned component_count = floats.size();
        ASSERT((component_count % 3) == 0);

        ASSERT(bytes.size() == (component_count / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                std::memcpy(to, from, 3 * sizeof(To));
                from += 3;
                to += 3 * sizeof(To);

                To alpha = 1;
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32g32b32a32_to_r8g8b8a8_srgb(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = uint8_t;

        unsigned component_count = floats.size();
        ASSERT((component_count % 4) == 0);

        ASSERT(bytes.size() == component_count * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = color_conversion::linear_float_to_linear_uint8<float>(*from++);
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32g32b32a32_to_r16g16b16a16(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        using To = uint16_t;

        unsigned component_count = floats.size();
        ASSERT((component_count % 4) == 0);

        ASSERT(bytes.size() == component_count * sizeof(To));

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = float_to_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = float_to_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = float_to_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = float_to_uint16(*from++);
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void conv_copy(const std::vector<float>& floats, const Span<std::byte>& bytes)
{
        unsigned component_count = floats.size();
        ASSERT(bytes.size() == (component_count * sizeof(float)));

        ASSERT(bytes.size() == data_size(floats));
        std::memcpy(bytes.data(), floats.data(), bytes.size());
}

void check_equal_component_count(ColorFormat from_format, ColorFormat to_format)
{
        if (component_count(from_format) != component_count(to_format))
        {
                component_count_error(from_format, to_format);
        }
}

void conv_src_to_floats(
        ColorFormat from_format,
        const Span<const std::byte>& from,
        ColorFormat to_format,
        std::vector<float>* pixels)
{
        switch (from_format)
        {
        case ColorFormat::R8_SRGB:
                check_equal_component_count(from_format, to_format);
                conv_r8_srgb_to_r32(from, pixels);
                return;
        case ColorFormat::R8G8B8_SRGB:
                check_equal_component_count(from_format, to_format);
                conv_r8g8b8_srgb_to_r32g32b32(from, pixels);
                return;
        case ColorFormat::R8G8B8A8_SRGB:
                switch (component_count(to_format))
                {
                case 4:
                        conv_r8g8b8a8_srgb_to_r32g32b32a32(from, pixels);
                        return;
                case 3:
                        conv_r8g8b8a8_srgb_to_r32g32b32(from, pixels);
                        return;
                default:
                        component_count_error(from_format, to_format);
                }
        case ColorFormat::R16:
                check_equal_component_count(from_format, to_format);
                conv_r16_to_r32(from, pixels);
                return;
        case ColorFormat::R16G16B16:
                check_equal_component_count(from_format, to_format);
                conv_r16g16b16_to_r32g32b32(from, pixels);
                return;
        case ColorFormat::R16G16B16A16:
                switch (component_count(to_format))
                {
                case 4:
                        conv_r16g16b16a16_to_r32g32b32a32(from, pixels);
                        return;
                case 3:
                        conv_r16g16b16a16_to_r32g32b32(from, pixels);
                        return;
                default:
                        component_count_error(from_format, to_format);
                }
        case ColorFormat::R32:
                check_equal_component_count(from_format, to_format);
                conv_copy(from, pixels);
                return;
        case ColorFormat::R32G32B32:
                check_equal_component_count(from_format, to_format);
                conv_copy(from, pixels);
                return;
        case ColorFormat::R32G32B32A32:
                switch (component_count(to_format))
                {
                case 4:
                        conv_copy(from, pixels);
                        return;
                case 3:
                        conv_r32g32b32a32_to_r32g32b32(from, pixels);
                        return;
                default:
                        component_count_error(from_format, to_format);
                }
        }
        unknown_color_format_error(from_format);
}

void conv_floats_to_dst(
        ColorFormat from_format,
        const std::vector<float>& pixels,
        ColorFormat to_format,
        const Span<std::byte>& to)
{
        switch (to_format)
        {
        case ColorFormat::R8_SRGB:
        {
                conv_r32_to_r8_srgb(pixels, to);
                return;
        }
        case ColorFormat::R8G8B8_SRGB:
                conv_r32g32b32_to_r8g8b8_srgb(pixels, to);
                return;
        case ColorFormat::R8G8B8A8_SRGB:
                switch (component_count(from_format))
                {
                case 4:
                        conv_r32g32b32a32_to_r8g8b8a8_srgb(pixels, to);
                        return;
                case 3:
                        conv_r32g32b32_to_r8g8b8a8_srgb(pixels, to);
                        return;
                default:
                        component_count_error(from_format, to_format);
                }
        case ColorFormat::R16:
                conv_r32_to_r16(pixels, to);
                return;
        case ColorFormat::R16G16B16:
                conv_r32g32b32_to_r16g16b16(pixels, to);
                return;
        case ColorFormat::R16G16B16A16:
                switch (component_count(from_format))
                {
                case 4:
                        conv_r32g32b32a32_to_r16g16b16a16(pixels, to);
                        return;
                case 3:
                        conv_r32g32b32_to_r16g16b16a16(pixels, to);
                        return;
                default:
                        component_count_error(from_format, to_format);
                }
        case ColorFormat::R32:
                conv_copy(pixels, to);
                return;
        case ColorFormat::R32G32B32:
                conv_copy(pixels, to);
                return;
        case ColorFormat::R32G32B32A32:
                switch (component_count(from_format))
                {
                case 4:
                        conv_copy(pixels, to);
                        return;
                case 3:
                        conv_r32g32b32_to_r32g32b32a32(pixels, to);
                        return;
                default:
                        component_count_error(from_format, to_format);
                }
        }
        unknown_color_format_error(to_format);
}

void conv(ColorFormat from_format, const Span<const std::byte>& from, ColorFormat to_format, const Span<std::byte>& to)
{
        std::vector<float> pixels;
        conv_src_to_floats(from_format, from, to_format, &pixels);
        conv_floats_to_dst(from_format, pixels, to_format, to);
}
}

std::string format_to_string(ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return "R8_SRGB";
        case ColorFormat::R8G8B8_SRGB:
                return "R8G8B8_SRGB";
        case ColorFormat::R8G8B8A8_SRGB:
                return "R8G8B8A8_SRGB";
        case ColorFormat::R16:
                return "R16";
        case ColorFormat::R16G16B16:
                return "R16G16B16";
        case ColorFormat::R16G16B16A16:
                return "R16G16B16A16";
        case ColorFormat::R32:
                return "R32";
        case ColorFormat::R32G32B32:
                return "R32G32B32";
        case ColorFormat::R32G32B32A32:
                return "R32G32B32A32";
        }
        unknown_color_format_error(format);
}

unsigned pixel_size_in_bytes(ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return 1;
        case ColorFormat::R8G8B8_SRGB:
                return 3;
        case ColorFormat::R8G8B8A8_SRGB:
                return 4;
        case ColorFormat::R16:
                return 2;
        case ColorFormat::R16G16B16:
                return 6;
        case ColorFormat::R16G16B16A16:
                return 8;
        case ColorFormat::R32:
                return 4;
        case ColorFormat::R32G32B32:
                return 12;
        case ColorFormat::R32G32B32A32:
                return 16;
        }
        unknown_color_format_error(format);
}

unsigned component_count(ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return 1;
        case ColorFormat::R8G8B8_SRGB:
                return 3;
        case ColorFormat::R8G8B8A8_SRGB:
                return 4;
        case ColorFormat::R16:
                return 1;
        case ColorFormat::R16G16B16:
                return 3;
        case ColorFormat::R16G16B16A16:
                return 4;
        case ColorFormat::R32:
                return 1;
        case ColorFormat::R32G32B32:
                return 3;
        case ColorFormat::R32G32B32A32:
                return 4;
        }
        unknown_color_format_error(format);
}

void format_conversion(
        ColorFormat from_format,
        const Span<const std::byte>& from,
        ColorFormat to_format,
        const Span<std::byte>& to)
{
        ASSERT(from.data());
        ASSERT(to.data());

        const std::lldiv_t pixel_count = std::lldiv(from.size(), pixel_size_in_bytes(from_format));

        if (pixel_count.rem != 0)
        {
                error("Error input color data size " + to_string(from.size()) + " for color format "
                      + format_to_string(from_format));
        }

        if (pixel_count.quot * pixel_size_in_bytes(to_format) != static_cast<long long>(to.size()))
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

        conv(from_format, from, to_format, to);
}

void format_conversion(
        ColorFormat from_format,
        const Span<const std::byte>& from,
        ColorFormat to_format,
        std::vector<std::byte>* to)
{
        ASSERT(to);
        to->resize((from.size() / pixel_size_in_bytes(from_format)) * pixel_size_in_bytes(to_format));

        conv(from_format, from, to_format, Span<std::byte>(data_pointer(*to), data_size(*to)));
}
}
