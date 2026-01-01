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

#include "format.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <string>

namespace ns::image
{
[[noreturn]] void unknown_color_format_error(const ColorFormat format)
{
        error_fatal("Unknown color format " + to_string(enum_to_int(format)));
}

std::string format_to_string(const ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return "R8_SRGB";
        case ColorFormat::R8G8B8_SRGB:
                return "R8G8B8_SRGB";
        case ColorFormat::R8G8B8A8_SRGB:
                return "R8G8B8A8_SRGB";
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                return "R8G8B8A8_SRGB_PREMULTIPLIED";
        case ColorFormat::R16:
                return "R16";
        case ColorFormat::R16G16B16:
                return "R16G16B16";
        case ColorFormat::R16G16B16_SRGB:
                return "R16G16B16_SRGB";
        case ColorFormat::R16G16B16A16:
                return "R16G16B16A16";
        case ColorFormat::R16G16B16A16_SRGB:
                return "R16G16B16A16_SRGB";
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                return "R16G16B16A16_PREMULTIPLIED";
        case ColorFormat::R32:
                return "R32";
        case ColorFormat::R32G32B32:
                return "R32G32B32";
        case ColorFormat::R32G32B32A32:
                return "R32G32B32A32";
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                return "R32G32B32A32_PREMULTIPLIED";
        }
        unknown_color_format_error(format);
}
}
