/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "conv_to_bytes.h"

#include <src/color/conversion.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace ns::image::conv
{
void r32_to_r8_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint8_t;

        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                const To value = color::linear_float_to_srgb_uint8<float>(*from++);
                std::memcpy(to, &value, sizeof(To));
                to += sizeof(To);
        }
}

void r32_to_r16(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                const To value = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &value, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r8g8b8_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint8_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_srgb_uint8<float>(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
        }
}

void r32g32b32_to_r8g8b8a8_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint8_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == (floats.size() / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        static constexpr To ALPHA = Limits<To>::max();
        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_srgb_uint8<float>(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
                std::memcpy(to, &ALPHA, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r16g16b16(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_linear_uint16(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
        }
}

void r32g32b32_to_r16g16b16_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_srgb_uint16(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
        }
}

void r32g32b32_to_r16g16b16a16(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == (floats.size() / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        static constexpr To ALPHA = Limits<To>::max();
        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_linear_uint16(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
                std::memcpy(to, &ALPHA, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r16g16b16a16_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == (floats.size() / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        static constexpr To ALPHA = Limits<To>::max();
        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_srgb_uint16(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
                std::memcpy(to, &ALPHA, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32_to_r32g32b32a32(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = float;

        ASSERT((floats.size() % 3) == 0);
        ASSERT(bytes.size() == (floats.size() / 3) * 4 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        static constexpr To ALPHA = 1;
        while (from != end)
        {
                std::memcpy(to, from, 3 * sizeof(To));
                from += 3;
                to += 3 * sizeof(To);

                std::memcpy(to, &ALPHA, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32a32_to_r8g8b8a8_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint8_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_srgb_uint8<float>(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
                const To alpha = color::linear_float_to_linear_uint8<float>(*from++);
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32a32_to_r8g8b8_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint8_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == (floats.size() / 4) * 3 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_srgb_uint8<float>(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
                ++from;
        }
}

void r32g32b32a32_to_r16g16b16a16(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 4; ++i)
                {
                        const To value = color::linear_float_to_linear_uint16(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
        }
}

void r32g32b32a32_to_r16g16b16a16_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == floats.size() * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_srgb_uint16(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
                const To alpha = color::linear_float_to_linear_uint16(*from++);
                std::memcpy(to, &alpha, sizeof(To));
                to += sizeof(To);
        }
}

void r32g32b32a32_to_r16g16b16(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == (floats.size() / 4) * 3 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_linear_uint16(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
                ++from;
        }
}

void r32g32b32a32_to_r16g16b16_srgb(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        using To = std::uint16_t;

        ASSERT((floats.size() % 4) == 0);
        ASSERT(bytes.size() == (floats.size() / 4) * 3 * sizeof(To));

        const float* from = floats.data();
        const float* const end = from + floats.size();
        std::byte* to = bytes.data();

        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const To value = color::linear_float_to_srgb_uint16(*from++);
                        std::memcpy(to, &value, sizeof(To));
                        to += sizeof(To);
                }
                ++from;
        }
}

void r32g32b32a32_to_r32g32b32(const std::vector<float>& floats, const std::span<std::byte> bytes)
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

void copy(const std::vector<float>& floats, const std::span<std::byte> bytes)
{
        ASSERT(bytes.size() == std::span(floats).size_bytes());

        std::memcpy(bytes.data(), floats.data(), bytes.size());
}
}
