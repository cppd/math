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

#include "format.h"

#include <src/com/error.h>
#include <src/com/print.h>

namespace image
{
namespace
{
template <typename T>
std::string enum_to_string(T e)
{
        static_assert(sizeof(e) <= sizeof(long long));

        return to_string(static_cast<long long>(e));
}

[[noreturn]] void unknown_color_format_error(ColorFormat format)
{
        error_fatal("Unknown color format " + enum_to_string(format));
}
}

std::string format_to_string(ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return "R8_SRGB";
        case ColorFormat::R8G8B8_SRGB:
                return "R8G8B8_SRGB";
        case ColorFormat::R8G8B8A8_SRGB:
                return "R8G8B8A8_SRGB";
        case ColorFormat::R16:
                return "R16";
        case ColorFormat::R16G16B16:
                return "R16G16B16";
        case ColorFormat::R16G16B16A16:
                return "R16G16B16A16";
        case ColorFormat::R32:
                return "R32";
        case ColorFormat::R32G32B32:
                return "R32G32B32";
        case ColorFormat::R32G32B32A32:
                return "R32G32B32A32";
        }
        unknown_color_format_error(format);
}

unsigned format_pixel_size_in_bytes(ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return 1;
        case ColorFormat::R8G8B8_SRGB:
                return 3;
        case ColorFormat::R8G8B8A8_SRGB:
                return 4;
        case ColorFormat::R16:
                return 2;
        case ColorFormat::R16G16B16:
                return 6;
        case ColorFormat::R16G16B16A16:
                return 8;
        case ColorFormat::R32:
                return 4;
        case ColorFormat::R32G32B32:
                return 12;
        case ColorFormat::R32G32B32A32:
                return 16;
        }
        unknown_color_format_error(format);
}

unsigned format_component_count(ColorFormat format)
{
        switch (format)
        {
        case ColorFormat::R8_SRGB:
                return 1;
        case ColorFormat::R8G8B8_SRGB:
                return 3;
        case ColorFormat::R8G8B8A8_SRGB:
                return 4;
        case ColorFormat::R16:
                return 1;
        case ColorFormat::R16G16B16:
                return 3;
        case ColorFormat::R16G16B16A16:
                return 4;
        case ColorFormat::R32:
                return 1;
        case ColorFormat::R32G32B32:
                return 3;
        case ColorFormat::R32G32B32A32:
                return 4;
        }
        unknown_color_format_error(format);
}
}
