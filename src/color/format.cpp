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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

namespace color
{
namespace
{
static_assert(sizeof(float) == sizeof(uint32_t));

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

template <typename T>
std::string enum_to_string(T e)
{
        static_assert(sizeof(e) <= sizeof(long long));

        return to_string(static_cast<long long>(e));
}

void conv_r8_srgb_to_r32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint8_t;

        unsigned component_count = bytes.size() / sizeof(From);
        floats->resize(component_count);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 1) == 0);

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
        floats->resize(component_count);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 1) == 0);

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
        floats->resize(component_count);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

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
        floats->resize(component_count);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

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
        floats->resize(component_count);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

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

void conv_r16g16b16a16_to_r32g32b32a32(const Span<const std::byte>& bytes, std::vector<float>* floats)
{
        using From = uint16_t;

        unsigned component_count = bytes.size() / sizeof(From);
        floats->resize(component_count);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

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

void conv_r32_to_r8_srgb(const std::vector<float>& floats, std::vector<std::byte>* bytes)
{
        using To = uint8_t;

        unsigned component_count = floats.size();
        bytes->resize(component_count * sizeof(To));
        ASSERT((component_count % 1) == 0);

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes->data();

        while (from != end)
        {
                To red = color_conversion::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32_to_r16(const std::vector<float>& floats, std::vector<std::byte>* bytes)
{
        using To = uint16_t;

        unsigned component_count = floats.size();
        bytes->resize(component_count * sizeof(To));
        ASSERT((component_count % 1) == 0);

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes->data();

        while (from != end)
        {
                To red = float_to_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);
        }
}

void conv_r32g32b32_to_r8g8b8_srgb(const std::vector<float>& floats, std::vector<std::byte>* bytes)
{
        using To = uint8_t;

        unsigned component_count = floats.size();
        bytes->resize(component_count * sizeof(To));
        ASSERT((component_count % 3) == 0);

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes->data();

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

void conv_r32g32b32_to_r16g16b16(const std::vector<float>& floats, std::vector<std::byte>* bytes)
{
        using To = uint16_t;

        unsigned component_count = floats.size();
        bytes->resize(component_count * sizeof(To));
        ASSERT((component_count % 3) == 0);

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes->data();

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

void conv_r32g32b32a32_to_r8g8b8a8_srgb(const std::vector<float>& floats, std::vector<std::byte>* bytes)
{
        using To = uint8_t;

        unsigned component_count = floats.size();
        bytes->resize(component_count * sizeof(To));
        ASSERT((component_count % 4) == 0);

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes->data();

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

void conv_r32g32b32a32_to_r16g16b16a16(const std::vector<float>& floats, std::vector<std::byte>* bytes)
{
        using To = uint16_t;

        unsigned component_count = floats.size();
        bytes->resize(component_count * sizeof(To));
        ASSERT((component_count % 4) == 0);

        const float* from = floats.data();
        const float* end = from + floats.size();
        std::byte* to = bytes->data();

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

void conv(ColorFormat from_format, const Span<const std::byte>& from, ColorFormat to_format, std::vector<std::byte>* to)
{
        std::vector<float> pixels;

        switch (from_format)
        {
        case ColorFormat::R8_SRGB:
                conv_r8_srgb_to_r32(from, &pixels);
                break;
        case ColorFormat::R8G8B8_SRGB:
                conv_r8g8b8_srgb_to_r32g32b32(from, &pixels);
                break;
        case ColorFormat::R8G8B8A8_SRGB:
                conv_r8g8b8a8_srgb_to_r32g32b32a32(from, &pixels);
                break;
        case ColorFormat::R16:
                conv_r16_to_r32(from, &pixels);
                break;
        case ColorFormat::R16G16B16:
                conv_r16g16b16_to_r32g32b32(from, &pixels);
                break;
        case ColorFormat::R16G16B16A16:
                conv_r16g16b16a16_to_r32g32b32a32(from, &pixels);
                break;
        case ColorFormat::R32:
        case ColorFormat::R32G32B32:
        case ColorFormat::R32G32B32A32:
                pixels.resize(from.size() / sizeof(float));
                ASSERT(pixels.size() * sizeof(float) == from.size());
                std::memcpy(pixels.data(), from.data(), from.size());
                break;
        }

        switch (to_format)
        {
        case ColorFormat::R8_SRGB:
        {
                conv_r32_to_r8_srgb(pixels, to);
                break;
        }
        case ColorFormat::R8G8B8_SRGB:
                conv_r32g32b32_to_r8g8b8_srgb(pixels, to);
                break;
        case ColorFormat::R8G8B8A8_SRGB:
                conv_r32g32b32a32_to_r8g8b8a8_srgb(pixels, to);
                break;
        case ColorFormat::R16:
                conv_r32_to_r16(pixels, to);
                break;
        case ColorFormat::R16G16B16:
                conv_r32g32b32_to_r16g16b16(pixels, to);
                break;
        case ColorFormat::R16G16B16A16:
                conv_r32g32b32a32_to_r16g16b16a16(pixels, to);
                break;
        case ColorFormat::R32:
        case ColorFormat::R32G32B32:
        case ColorFormat::R32G32B32A32:
                to->resize(pixels.size() * sizeof(float));
                ASSERT(to->size() / sizeof(float) == pixels.size());
                std::memcpy(to->data(), pixels.data(), to->size());
                break;
        }
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
        error_fatal("Unknown color format " + enum_to_string(format));
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
        error_fatal("Unknown color format " + enum_to_string(format));
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
        error_fatal("Unknown color format " + enum_to_string(format));
}

void format_conversion(
        ColorFormat from_format,
        const Span<const std::byte>& from,
        ColorFormat to_format,
        std::vector<std::byte>* to)
{
        ASSERT(to);

        if (from_format == to_format)
        {
                to->resize(from.size());
                std::memcpy(to->data(), from.data(), from.size());
                return;
        }

        unsigned one_pixel_size = pixel_size_in_bytes(from_format);

        if (from.size() % one_pixel_size)
        {
                error("Error size " + to_string(from.size()) + " for color format " + format_to_string(from_format));
        }

        if (component_count(from_format) != component_count(to_format))
        {
                error("Conversion between color types with different component count are not supported");
        }

        conv(from_format, from, to_format, to);
}
}
