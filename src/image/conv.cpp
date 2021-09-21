/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "conv.h"

#include <src/color/conversion.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <cstring>

namespace ns::image::conv
{
void r8_srgb_to_r32(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        using From = uint8_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 1) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        auto to = floats->begin();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color::srgb_uint8_to_linear_float(red);
                from += sizeof(From);
        }
}

void r16_to_r32(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        using From = uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 1) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        auto to = floats->begin();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(red);
                from += sizeof(From);
        }
}

void r8g8b8_srgb_to_r32g32b32(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        using From = uint8_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color::srgb_uint8_to_linear_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color::srgb_uint8_to_linear_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color::srgb_uint8_to_linear_float(blue);
                from += sizeof(From);
        }
}

void r16g16b16_to_r32g32b32(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        using From = uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(blue);
                from += sizeof(From);
        }
}

void r16g16b16_srgb_to_r32g32b32(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        using From = uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color::srgb_uint16_to_linear_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color::srgb_uint16_to_linear_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color::srgb_uint16_to_linear_float(blue);
                from += sizeof(From);
        }
}

void r8g8b8a8_srgb_to_r32g32b32a32(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        using From = uint8_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color::srgb_uint8_to_linear_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color::srgb_uint8_to_linear_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color::srgb_uint8_to_linear_float(blue);
                from += sizeof(From);

                From alpha;
                std::memcpy(&alpha, from, sizeof(From));
                *to++ = color::linear_uint8_to_linear_float(alpha);
                from += sizeof(From);
        }
}

void r16g16b16a16_to_r32g32b32a32(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        using From = uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(blue);
                from += sizeof(From);

                From alpha;
                std::memcpy(&alpha, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(alpha);
                from += sizeof(From);
        }
}

void r16g16b16a16_srgb_to_r32g32b32a32(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        using From = uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        while (from != end)
        {
                From red;
                std::memcpy(&red, from, sizeof(From));
                *to++ = color::srgb_uint16_to_linear_float(red);
                from += sizeof(From);

                From green;
                std::memcpy(&green, from, sizeof(From));
                *to++ = color::srgb_uint16_to_linear_float(green);
                from += sizeof(From);

                From blue;
                std::memcpy(&blue, from, sizeof(From));
                *to++ = color::srgb_uint16_to_linear_float(blue);
                from += sizeof(From);

                From alpha;
                std::memcpy(&alpha, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(alpha);
                from += sizeof(From);
        }
}

void copy(const std::span<const std::byte>& bytes, std::vector<float>* const floats)
{
        const unsigned component_count = bytes.size() / sizeof(float);
        ASSERT(bytes.size() == (component_count * sizeof(float)));

        floats->resize(component_count);

        ASSERT(std::span(*floats).size_bytes() == bytes.size());
        std::memcpy(floats->data(), bytes.data(), bytes.size());
}

//

void r32_to_r8_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint8_t;

        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);
        }
}

void r32_to_r16(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r8g8b8_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint8_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r8g8b8a8_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint8_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == (floats.size() / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = Limits<To>::max();
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r16g16b16(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r16g16b16_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r16g16b16a16(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == (floats.size() / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = Limits<To>::max();
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r16g16b16a16_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == (floats.size() / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = Limits<To>::max();
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r32g32b32a32(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = float;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == (floats.size() / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
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

void r32g32b32a32_to_r8g8b8a8_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint8_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = color::linear_float_to_linear_uint8<float>(*from++);
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32a32_to_r8g8b8_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint8_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == (floats.size() / 4) * 3 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                ++from;
        }
}

void r32g32b32a32_to_r16g16b16a16(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32a32_to_r16g16b16a16_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                To alpha = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32a32_to_r16g16b16(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == (floats.size() / 4) * 3 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                ++from;
        }
}

void r32g32b32a32_to_r16g16b16_srgb(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = uint16_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == (floats.size() / 4) * 3 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                To red = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &red, sizeof(To));
                to += sizeof(To);

                To green = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &green, sizeof(To));
                to += sizeof(To);

                To blue = color::linear_float_to_srgb_uint16(*from++);
                std::memcpy(to, &blue, sizeof(To));
                to += sizeof(To);

                ++from;
        }
}

void r32g32b32a32_to_r32g32b32(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        using To = float;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == (floats.size() / 4) * 3 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                std::memcpy(to, from, 3 * sizeof(To));
                from += 4;
                to += 3 * sizeof(To);
        }
}

void copy(const std::vector<float>& floats, const std::span<std::byte>& bytes)
{
        ASSERT(bytes.size() == std::span(floats).size_bytes());

        std::memcpy(bytes.data(), floats.data(), bytes.size());
}
}
