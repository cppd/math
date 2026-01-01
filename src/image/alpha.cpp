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

#include "alpha.h"

#include "alpha_blend.h"
#include "format.h"
#include "image.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace ns::image
{
namespace
{
template <typename T>
std::vector<std::byte> add_alpha(const std::span<const std::byte> bytes, const T alpha)
{
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

        if (bytes.size() % (3 * sizeof(T)) != 0)
        {
                error("Error data size (" + to_string(bytes.size()) + ") for adding alpha");
        }

        std::vector<std::byte> res(4 * (bytes.size() / 3));

        const std::byte* src = bytes.data();
        const std::byte* const src_end = src + bytes.size();
        std::byte* dst = res.data();
        while (src != src_end)
        {
                std::memcpy(dst, src, 3 * sizeof(T));
                dst += 3 * sizeof(T);
                std::memcpy(dst, &alpha, sizeof(T));
                dst += sizeof(T);
                src += 3 * sizeof(T);
        }
        ASSERT(dst == res.data() + res.size());

        return res;
}

template <typename T>
std::vector<std::byte> delete_alpha(const std::span<const std::byte> bytes)
{
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

        if (bytes.size() % (4 * sizeof(T)) != 0)
        {
                error("Error data size (" + to_string(bytes.size()) + ") for deleting alpha");
        }

        std::vector<std::byte> res(3 * (bytes.size() / 4));

        const std::byte* src = bytes.data();
        const std::byte* const src_end = src + bytes.size();
        std::byte* dst = res.data();
        while (src != src_end)
        {
                std::memcpy(dst, src, 3 * sizeof(T));
                src += 4 * sizeof(T);
                dst += 3 * sizeof(T);
        }
        ASSERT(dst == res.data() + res.size());

        return res;
}

template <typename T>
void set_alpha(const std::span<std::byte> bytes, const T alpha)
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
}

void blend_alpha(ColorFormat* const color_format, const std::span<std::byte>& bytes, numerical::Vector<3, float> rgb)
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

void set_alpha(const ColorFormat color_format, const std::span<std::byte>& bytes, float alpha)
{
        alpha = std::clamp<float>(alpha, 0, 1);

        switch (color_format)
        {
        case ColorFormat::R8G8B8A8_SRGB:
                set_alpha<std::uint8_t>(bytes, std::lround(alpha * Limits<std::uint8_t>::max()));
                return;
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
                set_alpha<std::uint16_t>(bytes, std::lround(alpha * Limits<std::uint16_t>::max()));
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
        Image<N> res;

        alpha = std::clamp(alpha, 0.0f, 1.0f);

        res.size = image.size;

        switch (image.color_format)
        {
        case ColorFormat::R8G8B8_SRGB:
                res.color_format = ColorFormat::R8G8B8A8_SRGB;
                res.pixels = add_alpha<std::uint8_t>(image.pixels, std::lround(alpha * Limits<std::uint8_t>::max()));
                return res;
        case ColorFormat::R16G16B16:
                res.color_format = ColorFormat::R16G16B16A16;
                res.pixels = add_alpha<std::uint16_t>(image.pixels, std::lround(alpha * Limits<std::uint16_t>::max()));
                return res;
        case ColorFormat::R16G16B16_SRGB:
                res.color_format = ColorFormat::R16G16B16A16_SRGB;
                res.pixels = add_alpha<std::uint16_t>(image.pixels, std::lround(alpha * Limits<std::uint16_t>::max()));
                return res;
        case ColorFormat::R32G32B32:
                res.color_format = ColorFormat::R32G32B32A32;
                res.pixels = add_alpha<float>(image.pixels, alpha);
                return res;
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
        Image<N> res;

        res.size = image.size;

        switch (image.color_format)
        {
        case ColorFormat::R8G8B8A8_SRGB:
                res.color_format = ColorFormat::R8G8B8_SRGB;
                res.pixels = delete_alpha<std::uint8_t>(image.pixels);
                return res;
        case ColorFormat::R16G16B16A16:
                res.color_format = ColorFormat::R16G16B16;
                res.pixels = delete_alpha<std::uint16_t>(image.pixels);
                return res;
        case ColorFormat::R16G16B16A16_SRGB:
                res.color_format = ColorFormat::R16G16B16_SRGB;
                res.pixels = delete_alpha<std::uint16_t>(image.pixels);
                return res;
        case ColorFormat::R32G32B32A32:
                res.color_format = ColorFormat::R32G32B32;
                res.pixels = delete_alpha<float>(image.pixels);
                return res;
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

#define TEMPLATE(N)                                              \
        template Image<(N)> add_alpha(const Image<(N)>&, float); \
        template Image<(N)> delete_alpha(const Image<(N)>&);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
