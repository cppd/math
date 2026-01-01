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

#pragma once

#include <string>

namespace ns::image
{
enum class ColorFormat
{
        R8_SRGB,
        R8G8B8_SRGB,
        R8G8B8A8_SRGB,
        R8G8B8A8_SRGB_PREMULTIPLIED,
        R16,
        R16G16B16,
        R16G16B16_SRGB,
        R16G16B16A16,
        R16G16B16A16_SRGB,
        R16G16B16A16_PREMULTIPLIED,
        R32,
        R32G32B32,
        R32G32B32A32,
        R32G32B32A32_PREMULTIPLIED
};

[[noreturn]] void unknown_color_format_error(ColorFormat format);

std::string format_to_string(ColorFormat format);

constexpr unsigned format_pixel_size_in_bytes(const ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return 1;
        case ColorFormat::R8G8B8_SRGB:
                return 3;
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                return 4;
        case ColorFormat::R16:
                return 2;
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
                return 6;
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                return 8;
        case ColorFormat::R32:
                return 4;
        case ColorFormat::R32G32B32:
                return 12;
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                return 16;
        }
        unknown_color_format_error(format);
}

constexpr unsigned format_component_count(const ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return 1;
        case ColorFormat::R8G8B8_SRGB:
                return 3;
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                return 4;
        case ColorFormat::R16:
                return 1;
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
                return 3;
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                return 4;
        case ColorFormat::R32:
                return 1;
        case ColorFormat::R32G32B32:
                return 3;
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                return 4;
        }
        unknown_color_format_error(format);
}

constexpr bool is_premultiplied(ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
        case ColorFormat::R8G8B8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R16:
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R32:
        case ColorFormat::R32G32B32:
        case ColorFormat::R32G32B32A32:
                return false;
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                return true;
        }
        unknown_color_format_error(format);
}
}
