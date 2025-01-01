/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "alpha_blend.h"

#include <src/color/conversion.h>
#include <src/com/error.h>
#include <src/com/interpolation.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace ns::image
{
void blend_alpha_r8g8b8a8(const std::span<std::byte> bytes, const numerical::Vector<3, float>& rgb)
{
        using T = std::uint8_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = Limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R8G8B8A8");
        }

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_srgb_uint8(rgb[0]);
                p[1] = color::linear_float_to_srgb_uint8(rgb[1]);
                p[2] = color::linear_float_to_srgb_uint8(rgb[2]);
                p[3] = DST_ALPHA;
                return p;
        }();

        std::byte* ptr = bytes.data();
        for (std::size_t i = 0; i < pixel_count; ++i, ptr += PIXEL_SIZE)
        {
                std::array<T, 4> pixel;
                std::memcpy(pixel.data(), ptr, PIXEL_SIZE);
                if (pixel[3] == 0)
                {
                        std::memcpy(ptr, blend_pixel.data(), PIXEL_SIZE);
                }
                else if (pixel[3] < Limits<T>::max())
                {
                        const float alpha = color::linear_uint8_to_linear_float(pixel[3]);
                        std::array<float, 3> c;
                        c[0] = color::srgb_uint8_to_linear_float(pixel[0]);
                        c[1] = color::srgb_uint8_to_linear_float(pixel[1]);
                        c[2] = color::srgb_uint8_to_linear_float(pixel[2]);
                        c[0] = interpolation(rgb[0], c[0], alpha);
                        c[1] = interpolation(rgb[1], c[1], alpha);
                        c[2] = interpolation(rgb[2], c[2], alpha);
                        pixel[0] = color::linear_float_to_srgb_uint8(c[0]);
                        pixel[1] = color::linear_float_to_srgb_uint8(c[1]);
                        pixel[2] = color::linear_float_to_srgb_uint8(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r8g8b8a8_premultiplied(const std::span<std::byte> bytes, const numerical::Vector<3, float>& rgb)
{
        using T = std::uint8_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = Limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R8G8B8A8");
        }

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_srgb_uint8(rgb[0]);
                p[1] = color::linear_float_to_srgb_uint8(rgb[1]);
                p[2] = color::linear_float_to_srgb_uint8(rgb[2]);
                p[3] = DST_ALPHA;
                return p;
        }();

        std::byte* ptr = bytes.data();
        for (std::size_t i = 0; i < pixel_count; ++i, ptr += PIXEL_SIZE)
        {
                std::array<T, 4> pixel;
                std::memcpy(pixel.data(), ptr, PIXEL_SIZE);
                if (pixel[3] == 0)
                {
                        std::memcpy(ptr, blend_pixel.data(), PIXEL_SIZE);
                }
                else if (pixel[3] < Limits<T>::max())
                {
                        const float alpha = color::linear_uint8_to_linear_float(pixel[3]);
                        const float k = 1 - alpha;
                        std::array<float, 3> c;
                        c[0] = color::srgb_uint8_to_linear_float(pixel[0]);
                        c[1] = color::srgb_uint8_to_linear_float(pixel[1]);
                        c[2] = color::srgb_uint8_to_linear_float(pixel[2]);
                        c[0] = k * rgb[0] + c[0];
                        c[1] = k * rgb[1] + c[1];
                        c[2] = k * rgb[2] + c[0];
                        pixel[0] = color::linear_float_to_srgb_uint8(c[0]);
                        pixel[1] = color::linear_float_to_srgb_uint8(c[1]);
                        pixel[2] = color::linear_float_to_srgb_uint8(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r16g16b16a16(const std::span<std::byte> bytes, const numerical::Vector<3, float>& rgb)
{
        using T = std::uint16_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = Limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R16G16B16A16");
        }

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_linear_uint16(rgb[0]);
                p[1] = color::linear_float_to_linear_uint16(rgb[1]);
                p[2] = color::linear_float_to_linear_uint16(rgb[2]);
                p[3] = DST_ALPHA;
                return p;
        }();

        std::byte* ptr = bytes.data();
        for (std::size_t i = 0; i < pixel_count; ++i, ptr += PIXEL_SIZE)
        {
                std::array<T, 4> pixel;
                std::memcpy(pixel.data(), ptr, PIXEL_SIZE);
                if (pixel[3] == 0)
                {
                        std::memcpy(ptr, blend_pixel.data(), PIXEL_SIZE);
                }
                else if (pixel[3] < Limits<T>::max())
                {
                        const float alpha = color::linear_uint16_to_linear_float(pixel[3]);
                        std::array<float, 3> c;
                        c[0] = color::linear_uint16_to_linear_float(pixel[0]);
                        c[1] = color::linear_uint16_to_linear_float(pixel[1]);
                        c[2] = color::linear_uint16_to_linear_float(pixel[2]);
                        c[0] = interpolation(rgb[0], c[0], alpha);
                        c[1] = interpolation(rgb[1], c[1], alpha);
                        c[2] = interpolation(rgb[2], c[2], alpha);
                        pixel[0] = color::linear_float_to_linear_uint16(c[0]);
                        pixel[1] = color::linear_float_to_linear_uint16(c[1]);
                        pixel[2] = color::linear_float_to_linear_uint16(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r16g16b16a16_srgb(const std::span<std::byte> bytes, const numerical::Vector<3, float>& rgb)
{
        using T = std::uint16_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = Limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R16G16B16A16");
        }

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_srgb_uint16(rgb[0]);
                p[1] = color::linear_float_to_srgb_uint16(rgb[1]);
                p[2] = color::linear_float_to_srgb_uint16(rgb[2]);
                p[3] = DST_ALPHA;
                return p;
        }();

        std::byte* ptr = bytes.data();
        for (std::size_t i = 0; i < pixel_count; ++i, ptr += PIXEL_SIZE)
        {
                std::array<T, 4> pixel;
                std::memcpy(pixel.data(), ptr, PIXEL_SIZE);
                if (pixel[3] == 0)
                {
                        std::memcpy(ptr, blend_pixel.data(), PIXEL_SIZE);
                }
                else if (pixel[3] < Limits<T>::max())
                {
                        const float alpha = color::linear_uint16_to_linear_float(pixel[3]);
                        std::array<float, 3> c;
                        c[0] = color::srgb_uint16_to_linear_float(pixel[0]);
                        c[1] = color::srgb_uint16_to_linear_float(pixel[1]);
                        c[2] = color::srgb_uint16_to_linear_float(pixel[2]);
                        c[0] = interpolation(rgb[0], c[0], alpha);
                        c[1] = interpolation(rgb[1], c[1], alpha);
                        c[2] = interpolation(rgb[2], c[2], alpha);
                        pixel[0] = color::linear_float_to_srgb_uint16(c[0]);
                        pixel[1] = color::linear_float_to_srgb_uint16(c[1]);
                        pixel[2] = color::linear_float_to_srgb_uint16(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r16g16b16a16_premultiplied(const std::span<std::byte> bytes, const numerical::Vector<3, float>& rgb)
{
        using T = std::uint16_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = Limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R16G16B16A16");
        }

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_linear_uint16(rgb[0]);
                p[1] = color::linear_float_to_linear_uint16(rgb[1]);
                p[2] = color::linear_float_to_linear_uint16(rgb[2]);
                p[3] = DST_ALPHA;
                return p;
        }();

        std::byte* ptr = bytes.data();
        for (std::size_t i = 0; i < pixel_count; ++i, ptr += PIXEL_SIZE)
        {
                std::array<T, 4> pixel;
                std::memcpy(pixel.data(), ptr, PIXEL_SIZE);
                if (pixel[3] == 0)
                {
                        std::memcpy(ptr, blend_pixel.data(), PIXEL_SIZE);
                }
                else if (pixel[3] < Limits<T>::max())
                {
                        const float alpha = color::linear_uint16_to_linear_float(pixel[3]);
                        const float k = 1 - alpha;
                        std::array<float, 3> c;
                        c[0] = color::linear_uint16_to_linear_float(pixel[0]);
                        c[1] = color::linear_uint16_to_linear_float(pixel[1]);
                        c[2] = color::linear_uint16_to_linear_float(pixel[2]);
                        c[0] = k * rgb[0] + c[0];
                        c[1] = k * rgb[1] + c[1];
                        c[2] = k * rgb[2] + c[2];
                        pixel[0] = color::linear_float_to_linear_uint16(c[0]);
                        pixel[1] = color::linear_float_to_linear_uint16(c[1]);
                        pixel[2] = color::linear_float_to_linear_uint16(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r32g32b32a32(const std::span<std::byte> bytes, const numerical::Vector<3, float>& rgb)
{
        using T = float;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr std::size_t COLOR_SIZE = 3 * sizeof(T);
        static constexpr std::size_t ALPHA_SIZE = 1 * sizeof(T);
        static constexpr T DST_ALPHA = 1;

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R32G32B32A32");
        }

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = rgb[0];
                p[1] = rgb[1];
                p[2] = rgb[2];
                p[3] = DST_ALPHA;
                return p;
        }();

        std::byte* ptr = bytes.data();
        for (std::size_t i = 0; i < pixel_count; ++i, ptr += PIXEL_SIZE)
        {
                std::array<T, 4> pixel;
                std::memcpy(pixel.data(), ptr, PIXEL_SIZE);
                if (pixel[3] <= 0)
                {
                        std::memcpy(ptr, blend_pixel.data(), PIXEL_SIZE);
                }
                else if (pixel[3] < 1)
                {
                        const float alpha = pixel[3];
                        pixel[0] = interpolation(blend_pixel[0], pixel[0], alpha);
                        pixel[1] = interpolation(blend_pixel[1], pixel[1], alpha);
                        pixel[2] = interpolation(blend_pixel[2], pixel[2], alpha);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
                else
                {
                        std::memcpy(ptr + COLOR_SIZE, &DST_ALPHA, ALPHA_SIZE);
                }
        }
}

void blend_alpha_r32g32b32a32_premultiplied(const std::span<std::byte> bytes, const numerical::Vector<3, float>& rgb)
{
        using T = float;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr std::size_t COLOR_SIZE = 3 * sizeof(T);
        static constexpr std::size_t ALPHA_SIZE = 1 * sizeof(T);
        static constexpr T DST_ALPHA = 1;

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R32G32B32A32_PREMULTIPLIED");
        }

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = rgb[0];
                p[1] = rgb[1];
                p[2] = rgb[2];
                p[3] = DST_ALPHA;
                return p;
        }();

        std::byte* ptr = bytes.data();
        for (std::size_t i = 0; i < pixel_count; ++i, ptr += PIXEL_SIZE)
        {
                std::array<T, 4> pixel;
                std::memcpy(pixel.data(), ptr, PIXEL_SIZE);
                if (pixel[3] <= 0)
                {
                        std::memcpy(ptr, blend_pixel.data(), PIXEL_SIZE);
                }
                else if (pixel[3] < 1)
                {
                        const float alpha = pixel[3];
                        const float k = 1 - alpha;
                        pixel[0] = k * blend_pixel[0] + pixel[0];
                        pixel[1] = k * blend_pixel[1] + pixel[1];
                        pixel[2] = k * blend_pixel[2] + pixel[2];
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
                else
                {
                        std::memcpy(ptr + COLOR_SIZE, &DST_ALPHA, ALPHA_SIZE);
                }
        }
}
}
