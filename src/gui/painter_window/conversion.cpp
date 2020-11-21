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

#include "conversion.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <cstring>

namespace gui
{
namespace
{
using BGRA32_TYPE = std::uint32_t;
constexpr size_t BGRA32_SIZE = sizeof(BGRA32_TYPE);

std::vector<std::byte> conv_bgra32_to_r8g8b8(const std::span<const std::byte>& pixels)
{
        std::vector<std::byte> bytes(3 * (pixels.size() / BGRA32_SIZE));
        std::byte* dst = bytes.data();
        for (auto src = pixels.begin(); src != pixels.end(); std::advance(src, BGRA32_SIZE))
        {
                BGRA32_TYPE pixel;
                std::memcpy(&pixel, &(*src), BGRA32_SIZE);
                unsigned char b = pixel & 0xff;
                unsigned char g = (pixel >> 8) & 0xff;
                unsigned char r = (pixel >> 16) & 0xff;
                std::memcpy(dst++, &r, 1);
                std::memcpy(dst++, &g, 1);
                std::memcpy(dst++, &b, 1);
        }
        return bytes;
}

std::vector<std::byte> conv_bgra32_to_r8g8b8a8(const std::span<const std::byte>& pixels)
{
        std::vector<std::byte> bytes(4 * (pixels.size() / BGRA32_SIZE));
        std::byte* dst = bytes.data();
        for (auto src = pixels.begin(); src != pixels.end(); std::advance(src, BGRA32_SIZE))
        {
                BGRA32_TYPE pixel;
                std::memcpy(&pixel, &(*src), BGRA32_SIZE);
                unsigned char b = pixel & 0xff;
                unsigned char g = (pixel >> 8) & 0xff;
                unsigned char r = (pixel >> 16) & 0xff;
                unsigned char a = (pixel >> 24) & 0xff;
                std::memcpy(dst++, &r, 1);
                std::memcpy(dst++, &g, 1);
                std::memcpy(dst++, &b, 1);
                std::memcpy(dst++, &a, 1);
        }
        return bytes;
}
}

std::vector<std::byte> format_conversion_from_bgra32(
        const std::span<const std::byte>& pixels,
        image::ColorFormat to_format)
{
        if (pixels.size() % BGRA32_SIZE != 0)
        {
                error("Error byte count (" + to_string(pixels.size()) + " for format BGRA32");
        }

        if (to_format == image::ColorFormat::R8G8B8_SRGB)
        {
                return conv_bgra32_to_r8g8b8(pixels);
        }
        if (to_format == image::ColorFormat::R8G8B8A8_SRGB)
        {
                return conv_bgra32_to_r8g8b8a8(pixels);
        }

        error("Unsupported format conversion from BGRA32 to " + image::format_to_string(to_format));
}
}
