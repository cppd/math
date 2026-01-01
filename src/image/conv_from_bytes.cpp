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

#include "conv_from_bytes.h"

#include <src/color/conversion.h>
#include <src/com/error.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace ns::image::conv
{
void r8_srgb_to_r32(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        using From = std::uint8_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 1) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        auto to = floats->begin();

        From value = 0;
        while (from != end)
        {
                std::memcpy(&value, from, sizeof(From));
                *to++ = color::srgb_uint8_to_linear_float(value);
                from += sizeof(From);
        }
}

void r16_to_r32(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        using From = std::uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 1) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        auto to = floats->begin();

        From value = 0;
        while (from != end)
        {
                std::memcpy(&value, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(value);
                from += sizeof(From);
        }
}

void r8g8b8_srgb_to_r32g32b32(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        using From = std::uint8_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        From value = 0;
        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        std::memcpy(&value, from, sizeof(From));
                        *to++ = color::srgb_uint8_to_linear_float(value);
                        from += sizeof(From);
                }
        }
}

void r16g16b16_to_r32g32b32(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        using From = std::uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        From value = 0;
        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        std::memcpy(&value, from, sizeof(From));
                        *to++ = color::linear_uint16_to_linear_float(value);
                        from += sizeof(From);
                }
        }
}

void r16g16b16_srgb_to_r32g32b32(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        using From = std::uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 3) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        From value = 0;
        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        std::memcpy(&value, from, sizeof(From));
                        *to++ = color::srgb_uint16_to_linear_float(value);
                        from += sizeof(From);
                }
        }
}

void r8g8b8a8_srgb_to_r32g32b32a32(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        using From = std::uint8_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        From value = 0;
        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        std::memcpy(&value, from, sizeof(From));
                        *to++ = color::srgb_uint8_to_linear_float(value);
                        from += sizeof(From);
                }
                std::memcpy(&value, from, sizeof(From));
                *to++ = color::linear_uint8_to_linear_float(value);
                from += sizeof(From);
        }
}

void r16g16b16a16_to_r32g32b32a32(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        using From = std::uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        From value = 0;
        while (from != end)
        {
                for (int i = 0; i < 4; ++i)
                {
                        std::memcpy(&value, from, sizeof(From));
                        *to++ = color::linear_uint16_to_linear_float(value);
                        from += sizeof(From);
                }
        }
}

void r16g16b16a16_srgb_to_r32g32b32a32(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        using From = std::uint16_t;

        const unsigned component_count = bytes.size() / sizeof(From);
        ASSERT(bytes.size() == (component_count * sizeof(From)));
        ASSERT((component_count % 4) == 0);

        floats->resize(component_count);

        const std::byte* from = bytes.data();
        const std::byte* const end = from + bytes.size();
        float* to = floats->data();

        From value = 0;
        while (from != end)
        {
                for (int i = 0; i < 3; ++i)
                {
                        std::memcpy(&value, from, sizeof(From));
                        *to++ = color::srgb_uint16_to_linear_float(value);
                        from += sizeof(From);
                }
                std::memcpy(&value, from, sizeof(From));
                *to++ = color::linear_uint16_to_linear_float(value);
                from += sizeof(From);
        }
}

void copy(const std::span<const std::byte> bytes, std::vector<float>* const floats)
{
        const unsigned component_count = bytes.size() / sizeof(float);
        ASSERT(bytes.size() == (component_count * sizeof(float)));

        floats->resize(component_count);

        ASSERT(std::span(*floats).size_bytes() == bytes.size());
        std::memcpy(floats->data(), bytes.data(), bytes.size());
}
}
