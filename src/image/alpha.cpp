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

#include "alpha.h"

#include <src/color/conversion.h>
#include <src/com/error.h>
#include <src/com/math.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

namespace ns::image
{
namespace
{
template <typename T>
std::vector<std::byte> add_alpha(const std::span<const std::byte>& bytes, T alpha)
{
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

        if (bytes.size() % (3 * sizeof(T)) != 0)
        {
                error("Error data size (" + to_string(bytes.size()) + ") for adding alpha");
        }

        std::vector<std::byte> result(4 * (bytes.size() / 3));

        const std::byte* src = bytes.data();
        const std::byte* const src_end = src + bytes.size();
        std::byte* dst = result.data();
        while (src != src_end)
        {
                std::memcpy(dst, src, 3 * sizeof(T));
                dst += 3 * sizeof(T);
                std::memcpy(dst, &alpha, sizeof(T));
                dst += sizeof(T);
                src += 3 * sizeof(T);
        }
        ASSERT(dst == result.data() + result.size());

        return result;
}

template <typename T>
std::vector<std::byte> delete_alpha(const std::span<const std::byte>& bytes)
{
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

        if (bytes.size() % (4 * sizeof(T)) != 0)
        {
                error("Error data size (" + to_string(bytes.size()) + ") for deleting alpha");
        }

        std::vector<std::byte> result(3 * (bytes.size() / 4));

        const std::byte* src = bytes.data();
        const std::byte* const src_end = src + bytes.size();
        std::byte* dst = result.data();
        while (src != src_end)
        {
                std::memcpy(dst, src, 3 * sizeof(T));
                src += 4 * sizeof(T);
                dst += 3 * sizeof(T);
        }
        ASSERT(dst == result.data() + result.size());

        return result;
}

template <typename T>
void set_alpha(const std::span<std::byte>& bytes, T alpha)
{
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

        if (bytes.size() % (4 * sizeof(T)) != 0)
        {
                error("Error data size " + to_string(bytes.size()) + " for setting alpha");
        }

        const std::size_t pixel_count = bytes.size() / (4 * sizeof(T));

        std::byte* ptr = bytes.data() + 3 * sizeof(T);
        for (std::size_t i = 0; i < pixel_count; ++i)
        {
                std::memcpy(ptr, &alpha, sizeof(T));
                ptr += 4 * sizeof(T);
        }
}

void blend_alpha_r8g8b8a8(const std::span<std::byte>& bytes, const Vector<3, float>& rgb)
{
        using T = uint8_t;

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
                        float alpha = color::linear_uint8_to_linear_float(pixel[3]);
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

void blend_alpha_r8g8b8a8_premultiplied(const std::span<std::byte>& bytes, const Vector<3, float>& rgb)
{
        using T = uint8_t;

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
                        float alpha = color::linear_uint8_to_linear_float(pixel[3]);
                        float k = 1 - alpha;
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

void blend_alpha_r16g16b16a16(const std::span<std::byte>& bytes, const Vector<3, float>& rgb)
{
        using T = uint16_t;

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
                        float alpha = color::linear_uint16_to_linear_float(pixel[3]);
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

void blend_alpha_r16g16b16a16_srgb(const std::span<std::byte>& bytes, const Vector<3, float>& rgb)
{
        using T = uint16_t;

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
                        float alpha = color::linear_uint16_to_linear_float(pixel[3]);
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

void blend_alpha_r16g16b16a16_premultiplied(const std::span<std::byte>& bytes, const Vector<3, float>& rgb)
{
        using T = uint16_t;

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
                        float alpha = color::linear_uint16_to_linear_float(pixel[3]);
                        float k = 1 - alpha;
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

void blend_alpha_r32g32b32a32(const std::span<std::byte>& bytes, const Vector<3, float>& rgb)
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
                        float alpha = pixel[3];
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

void blend_alpha_r32g32b32a32_premultiplied(const std::span<std::byte>& bytes, const Vector<3, float>& rgb)
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
                        float alpha = pixel[3];
                        float k = 1 - alpha;
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

void blend_alpha(ColorFormat* color_format, const std::span<std::byte>& bytes, Vector<3, float> rgb)
{
        rgb[0] = std::clamp<float>(rgb[0], 0, 1);
        rgb[1] = std::clamp<float>(rgb[1], 0, 1);
        rgb[2] = std::clamp<float>(rgb[2], 0, 1);

        switch (*color_format)
        {
        case ColorFormat::R8G8B8A8_SRGB:
                blend_alpha_r8g8b8a8(bytes, rgb);
                return;
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                blend_alpha_r8g8b8a8_premultiplied(bytes, rgb);
                *color_format = ColorFormat::R8G8B8A8_SRGB;
                return;
        case ColorFormat::R16G16B16A16:
                blend_alpha_r16g16b16a16(bytes, rgb);
                return;
        case ColorFormat::R16G16B16A16_SRGB:
                blend_alpha_r16g16b16a16_srgb(bytes, rgb);
                return;
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                blend_alpha_r16g16b16a16_premultiplied(bytes, rgb);
                *color_format = ColorFormat::R16G16B16A16;
                return;
        case ColorFormat::R32G32B32A32:
                blend_alpha_r32g32b32a32(bytes, rgb);
                return;
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                blend_alpha_r32g32b32a32_premultiplied(bytes, rgb);
                *color_format = ColorFormat::R32G32B32A32;
                return;
        case ColorFormat::R8G8B8_SRGB:
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
        case ColorFormat::R32G32B32:
        case ColorFormat::R16:
        case ColorFormat::R32:
        case ColorFormat::R8_SRGB:
                break;
        }

        error("Unsupported image format " + format_to_string(*color_format) + " for blending alpha");
}

void set_alpha(ColorFormat color_format, const std::span<std::byte>& bytes, float alpha)
{
        alpha = std::clamp<float>(alpha, 0, 1);

        switch (color_format)
        {
        case ColorFormat::R8G8B8A8_SRGB:
                set_alpha<uint8_t>(bytes, std::lround(alpha * Limits<uint8_t>::max()));
                return;
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
                set_alpha<uint16_t>(bytes, std::lround(alpha * Limits<uint16_t>::max()));
                return;
        case ColorFormat::R32G32B32A32:
                set_alpha<float>(bytes, alpha);
                return;
        case ColorFormat::R8G8B8_SRGB:
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
        case ColorFormat::R32G32B32:
        case ColorFormat::R16:
        case ColorFormat::R32:
        case ColorFormat::R8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                break;
        }

        error("Unsupported image format " + format_to_string(color_format) + " for setting alpha");
}

template <std::size_t N>
Image<N> add_alpha(const Image<N>& image, float alpha)
{
        Image<N> result;

        alpha = std::clamp(alpha, 0.0f, 1.0f);

        result.size = image.size;

        switch (image.color_format)
        {
        case ColorFormat::R8G8B8_SRGB:
                result.color_format = ColorFormat::R8G8B8A8_SRGB;
                result.pixels = add_alpha<uint8_t>(image.pixels, std::lround(alpha * Limits<uint8_t>::max()));
                return result;
        case ColorFormat::R16G16B16:
                result.color_format = ColorFormat::R16G16B16A16;
                result.pixels = add_alpha<uint16_t>(image.pixels, std::lround(alpha * Limits<uint16_t>::max()));
                return result;
        case ColorFormat::R16G16B16_SRGB:
                result.color_format = ColorFormat::R16G16B16A16_SRGB;
                result.pixels = add_alpha<uint16_t>(image.pixels, std::lround(alpha * Limits<uint16_t>::max()));
                return result;
        case ColorFormat::R32G32B32:
                result.color_format = ColorFormat::R32G32B32A32;
                result.pixels = add_alpha<float>(image.pixels, alpha);
                return result;
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R16:
        case ColorFormat::R32:
        case ColorFormat::R8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                break;
        }

        error("Unsupported image format " + format_to_string(image.color_format) + " for adding alpha");
}

template <std::size_t N>
Image<N> delete_alpha(const Image<N>& image)
{
        Image<N> result;

        result.size = image.size;

        switch (image.color_format)
        {
        case ColorFormat::R8G8B8A8_SRGB:
                result.color_format = ColorFormat::R8G8B8_SRGB;
                result.pixels = delete_alpha<uint8_t>(image.pixels);
                return result;
        case ColorFormat::R16G16B16A16:
                result.color_format = ColorFormat::R16G16B16;
                result.pixels = delete_alpha<uint16_t>(image.pixels);
                return result;
        case ColorFormat::R16G16B16A16_SRGB:
                result.color_format = ColorFormat::R16G16B16_SRGB;
                result.pixels = delete_alpha<uint16_t>(image.pixels);
                return result;
        case ColorFormat::R32G32B32A32:
                result.color_format = ColorFormat::R32G32B32;
                result.pixels = delete_alpha<float>(image.pixels);
                return result;
        case ColorFormat::R8G8B8_SRGB:
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
        case ColorFormat::R32G32B32:
        case ColorFormat::R16:
        case ColorFormat::R32:
        case ColorFormat::R8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                break;
        }

        error("Unsupported image format " + format_to_string(image.color_format) + " for deleting alpha");
}

template Image<2> add_alpha(const Image<2>&, float);
template Image<3> add_alpha(const Image<3>&, float);
template Image<4> add_alpha(const Image<4>&, float);
template Image<5> add_alpha(const Image<5>&, float);

template Image<2> delete_alpha(const Image<2>&);
template Image<3> delete_alpha(const Image<3>&);
template Image<4> delete_alpha(const Image<4>&);
template Image<5> delete_alpha(const Image<5>&);
}
