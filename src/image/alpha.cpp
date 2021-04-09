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

#include <src/com/math.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

namespace ns::image
{
namespace
{
template <typename T>
void add_alpha(const std::span<const std::byte>& bytes3, const std::span<std::byte>& bytes4, T alpha)
{
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

        if (bytes3.size() % (3 * sizeof(T)) != 0)
        {
                error("Error RGB data size " + to_string(bytes3.size()) + " for adding alpha");
        }

        if (bytes4.size() % (4 * sizeof(T)) != 0)
        {
                error("Error RGBA data size " + to_string(bytes4.size()) + " for adding alpha");
        }

        if ((bytes4.size() / 4) * 3 != bytes3.size() || (bytes3.size() / 3) * 4 != bytes4.size())
        {
                error("Error sizes for adding alpha: RGB data size = " + to_string(bytes3.size())
                      + ", RGBA data size = " + to_string(bytes4.size()));
        }

        const std::size_t pixel_count = bytes3.size() / (3 * sizeof(T));
        ASSERT(pixel_count * (3 * sizeof(T)) == bytes3.size());

        const std::byte* src = bytes3.data();
        std::byte* dst = bytes4.data();
        for (std::size_t i = 0; i < pixel_count; ++i)
        {
                std::memcpy(dst, src, 3 * sizeof(T));
                dst += 3 * sizeof(T);
                std::memcpy(dst, &alpha, sizeof(T));
                dst += sizeof(T);
                src += 3 * sizeof(T);
        }
}

void add_alpha(
        ColorFormat color_format,
        const std::span<const std::byte>& bytes3,
        const std::span<std::byte>& bytes4,
        float alpha)
{
        if (format_component_count(color_format) != 3)
        {
                error("Error image format " + format_to_string(color_format) + " for adding alpha");
        }

        alpha = std::clamp(alpha, 0.0f, 1.0f);

        if (color_format == ColorFormat::R16G16B16)
        {
                add_alpha<uint16_t>(bytes3, bytes4, std::lround(alpha * limits<uint16_t>::max()));
        }
        else if (color_format == ColorFormat::R32G32B32)
        {
                add_alpha<float>(bytes3, bytes4, alpha);
        }
        else if (color_format == ColorFormat::R8G8B8_SRGB)
        {
                add_alpha<uint8_t>(bytes3, bytes4, std::lround(alpha * limits<uint8_t>::max()));
        }
        else
        {
                error("Unsupported image format " + format_to_string(color_format) + " for adding alpha");
        }
}

template <typename T>
void set_alpha(const std::span<std::byte>& bytes, T alpha)
{
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

        if (bytes.size() % (4 * sizeof(T)) != 0)
        {
                error("Error RGBA data size " + to_string(bytes.size()) + " for setting alpha");
        }

        const std::size_t pixel_count = bytes.size() / (4 * sizeof(T));

        std::byte* ptr = bytes.data() + 3 * sizeof(T);
        for (std::size_t i = 0; i < pixel_count; ++i)
        {
                std::memcpy(ptr, &alpha, sizeof(T));
                ptr += 4 * sizeof(T);
        }
}

void blend_alpha_r8g8b8a8(const std::span<std::byte>& bytes, const Color& blend_color)
{
        using T = uint8_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R8G8B8A8");
        }

        const Vector<3, float>& blend_color_rgb = blend_color.rgb<float>();

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_srgb_uint8(blend_color_rgb[0]);
                p[1] = color::linear_float_to_srgb_uint8(blend_color_rgb[1]);
                p[2] = color::linear_float_to_srgb_uint8(blend_color_rgb[2]);
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
                else if (pixel[3] < limits<T>::max())
                {
                        float alpha = color::linear_uint8_to_linear_float(pixel[3]);
                        std::array<float, 3> c;
                        c[0] = color::srgb_uint8_to_linear_float(pixel[0]);
                        c[1] = color::srgb_uint8_to_linear_float(pixel[1]);
                        c[2] = color::srgb_uint8_to_linear_float(pixel[2]);
                        c[0] = interpolation(blend_color_rgb[0], c[0], alpha);
                        c[1] = interpolation(blend_color_rgb[1], c[1], alpha);
                        c[2] = interpolation(blend_color_rgb[2], c[2], alpha);
                        pixel[0] = color::linear_float_to_srgb_uint8(c[0]);
                        pixel[1] = color::linear_float_to_srgb_uint8(c[1]);
                        pixel[2] = color::linear_float_to_srgb_uint8(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r8g8b8a8_premultiplied(const std::span<std::byte>& bytes, const Color& blend_color)
{
        using T = uint8_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R8G8B8A8");
        }

        const Vector<3, float>& blend_color_rgb = blend_color.rgb<float>();

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_srgb_uint8(blend_color_rgb[0]);
                p[1] = color::linear_float_to_srgb_uint8(blend_color_rgb[1]);
                p[2] = color::linear_float_to_srgb_uint8(blend_color_rgb[2]);
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
                else if (pixel[3] < limits<T>::max())
                {
                        float alpha = color::linear_uint8_to_linear_float(pixel[3]);
                        float k = 1 - alpha;
                        std::array<float, 3> c;
                        c[0] = color::srgb_uint8_to_linear_float(pixel[0]);
                        c[1] = color::srgb_uint8_to_linear_float(pixel[1]);
                        c[2] = color::srgb_uint8_to_linear_float(pixel[2]);
                        c[0] = k * blend_color_rgb[0] + c[0];
                        c[1] = k * blend_color_rgb[1] + c[1];
                        c[2] = k * blend_color_rgb[2] + c[0];
                        pixel[0] = color::linear_float_to_srgb_uint8(c[0]);
                        pixel[1] = color::linear_float_to_srgb_uint8(c[1]);
                        pixel[2] = color::linear_float_to_srgb_uint8(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r16g16b16a16(const std::span<std::byte>& bytes, const Color& blend_color)
{
        using T = uint16_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R16G16B16A16");
        }

        const Vector<3, float>& blend_color_rgb = blend_color.rgb<float>();

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_linear_uint16(blend_color_rgb[0]);
                p[1] = color::linear_float_to_linear_uint16(blend_color_rgb[1]);
                p[2] = color::linear_float_to_linear_uint16(blend_color_rgb[2]);
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
                else if (pixel[3] < limits<T>::max())
                {
                        float alpha = color::linear_uint16_to_linear_float(pixel[3]);
                        std::array<float, 3> c;
                        c[0] = color::linear_uint16_to_linear_float(pixel[0]);
                        c[1] = color::linear_uint16_to_linear_float(pixel[1]);
                        c[2] = color::linear_uint16_to_linear_float(pixel[2]);
                        c[0] = interpolation(blend_color_rgb[0], c[0], alpha);
                        c[1] = interpolation(blend_color_rgb[1], c[1], alpha);
                        c[2] = interpolation(blend_color_rgb[2], c[2], alpha);
                        pixel[0] = color::linear_float_to_linear_uint16(c[0]);
                        pixel[1] = color::linear_float_to_linear_uint16(c[1]);
                        pixel[2] = color::linear_float_to_linear_uint16(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r16g16b16a16_premultiplied(const std::span<std::byte>& bytes, const Color& blend_color)
{
        using T = uint16_t;

        static constexpr std::size_t PIXEL_SIZE = 4 * sizeof(T);
        static constexpr T DST_ALPHA = limits<T>::max();

        const std::size_t pixel_count = bytes.size() / PIXEL_SIZE;
        if (pixel_count * PIXEL_SIZE != bytes.size())
        {
                error("Error size " + to_string(bytes.size()) + " for blending R16G16B16A16");
        }

        const Vector<3, float>& blend_color_rgb = blend_color.rgb<float>();

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = color::linear_float_to_linear_uint16(blend_color_rgb[0]);
                p[1] = color::linear_float_to_linear_uint16(blend_color_rgb[1]);
                p[2] = color::linear_float_to_linear_uint16(blend_color_rgb[2]);
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
                else if (pixel[3] < limits<T>::max())
                {
                        float alpha = color::linear_uint16_to_linear_float(pixel[3]);
                        float k = 1 - alpha;
                        std::array<float, 3> c;
                        c[0] = color::linear_uint16_to_linear_float(pixel[0]);
                        c[1] = color::linear_uint16_to_linear_float(pixel[1]);
                        c[2] = color::linear_uint16_to_linear_float(pixel[2]);
                        c[0] = k * blend_color_rgb[0] + c[0];
                        c[1] = k * blend_color_rgb[1] + c[1];
                        c[2] = k * blend_color_rgb[2] + c[2];
                        pixel[0] = color::linear_float_to_linear_uint16(c[0]);
                        pixel[1] = color::linear_float_to_linear_uint16(c[1]);
                        pixel[2] = color::linear_float_to_linear_uint16(c[2]);
                        pixel[3] = DST_ALPHA;
                        std::memcpy(ptr, pixel.data(), PIXEL_SIZE);
                }
        }
}

void blend_alpha_r32g32b32a32(const std::span<std::byte>& bytes, const Color& blend_color)
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

        const Vector<3, float>& blend_color_rgb = blend_color.rgb<float>();

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = blend_color_rgb[0];
                p[1] = blend_color_rgb[1];
                p[2] = blend_color_rgb[2];
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

void blend_alpha_r32g32b32a32_premultiplied(const std::span<std::byte>& bytes, const Color& blend_color)
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

        const Vector<3, float>& blend_color_rgb = blend_color.rgb<float>();

        const std::array<T, 4> blend_pixel = [&]
        {
                std::array<T, 4> p;
                p[0] = blend_color_rgb[0];
                p[1] = blend_color_rgb[1];
                p[2] = blend_color_rgb[2];
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

std::vector<std::byte> add_alpha(ColorFormat color_format, const std::span<const std::byte>& bytes, float alpha)
{
        if (!(color_format == ColorFormat::R8G8B8_SRGB || color_format == ColorFormat::R16G16B16
              || color_format == ColorFormat::R32G32B32))
        {
                error("Unsupported image format " + format_to_string(color_format) + " for adding alpha");
        }

        std::vector<std::byte> bytes4;

        bytes4.resize((bytes.size() / 3) * 4);
        add_alpha(color_format, bytes, bytes4, alpha);

        return bytes4;
}

std::vector<std::byte> delete_alpha(ColorFormat color_format, const std::span<const std::byte>& bytes)
{
        if (!(color_format == ColorFormat::R8G8B8A8_SRGB || color_format == ColorFormat::R16G16B16A16
              || color_format == ColorFormat::R32G32B32A32))
        {
                error("Unsupported image format " + format_to_string(color_format) + " for deleting alpha");
        }

        std::size_t src_pixel_size = format_pixel_size_in_bytes(color_format);
        if (bytes.size() % src_pixel_size != 0)
        {
                error("Error byte count (" + to_string(bytes.size()) + ") for format "
                      + format_to_string(color_format));
        }

        ASSERT(bytes.size() % 4 == 0);
        ASSERT(src_pixel_size % 4 == 0);
        std::vector<std::byte> result(3 * (bytes.size() / 4));
        std::size_t dst_pixel_size = 3 * (src_pixel_size / 4);

        auto src = bytes.begin();
        auto dst = result.begin();
        for (; src != bytes.end(); std::advance(src, src_pixel_size), std::advance(dst, dst_pixel_size))
        {
                std::memcpy(&(*dst), &(*src), dst_pixel_size);
        }

        return result;
}

void blend_alpha(ColorFormat* color_format, const std::span<std::byte>& bytes, const Color& color)
{
        if (*color_format == ColorFormat::R8G8B8A8_SRGB)
        {
                blend_alpha_r8g8b8a8(bytes, color);
        }
        else if (*color_format == ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED)
        {
                blend_alpha_r8g8b8a8_premultiplied(bytes, color);
                *color_format = ColorFormat::R8G8B8A8_SRGB;
        }
        else if (*color_format == ColorFormat::R16G16B16A16)
        {
                blend_alpha_r16g16b16a16(bytes, color);
        }
        else if (*color_format == ColorFormat::R16G16B16A16_PREMULTIPLIED)
        {
                blend_alpha_r16g16b16a16_premultiplied(bytes, color);
                *color_format = ColorFormat::R16G16B16A16;
        }
        else if (*color_format == ColorFormat::R32G32B32A32)
        {
                blend_alpha_r32g32b32a32(bytes, color);
        }
        else if (*color_format == ColorFormat::R32G32B32A32_PREMULTIPLIED)
        {
                blend_alpha_r32g32b32a32_premultiplied(bytes, color);
                *color_format = ColorFormat::R32G32B32A32;
        }
        else
        {
                error("Unsupported image format " + format_to_string(*color_format) + " for blending alpha");
        }
}

void set_alpha(ColorFormat color_format, const std::span<std::byte>& bytes, float alpha)
{
        if (format_component_count(color_format) != 4)
        {
                error("Error image format " + format_to_string(color_format) + " for setting alpha");
        }

        alpha = std::clamp(alpha, 0.0f, 1.0f);

        if (color_format == ColorFormat::R16G16B16A16)
        {
                set_alpha<uint16_t>(bytes, std::lround(alpha * limits<uint16_t>::max()));
        }
        else if (color_format == ColorFormat::R32G32B32A32)
        {
                set_alpha<float>(bytes, alpha);
        }
        else if (color_format == ColorFormat::R8G8B8A8_SRGB)
        {
                set_alpha<uint8_t>(bytes, std::lround(alpha * limits<uint8_t>::max()));
        }
        else
        {
                error("Unsupported image format " + format_to_string(color_format) + " for setting alpha");
        }
}
}
