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

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <cstring>

namespace gui::painter_window
{
namespace
{
std::vector<std::byte> conv_bgra_to_rgb(const std::span<const std::byte>& pixels)
{
        if (pixels.size() % 4 != 0)
        {
                error("Error byte count (" + to_string(pixels.size()) + " for format BGRA");
        }
        std::vector<std::byte> result(3 * (pixels.size() / 4));
        auto src = pixels.begin();
        auto dst = result.begin();
        for (; src != pixels.end(); std::advance(src, 4), std::advance(dst, 3))
        {
                std::memcpy(&(*dst), &(*src), 3);
                std::swap(*dst, *(dst + 2));
        }
        return result;
}

std::vector<std::byte> conv_bgra_to_rgba(const std::span<const std::byte>& pixels)
{
        if (pixels.size() % 4 != 0)
        {
                error("Error byte count (" + to_string(pixels.size()) + " for format BGRA");
        }
        std::vector<std::byte> result(pixels.size());
        std::memcpy(result.data(), pixels.data(), data_size(result));
        for (auto dst = result.begin(); dst != result.end(); std::advance(dst, 4))
        {
                std::swap(*dst, *(dst + 2));
        }
        return result;
}
}

std::vector<std::byte> format_conversion_from_bgra(
        const std::span<const std::byte>& pixels,
        image::ColorFormat to_format)
{
        if (to_format == image::ColorFormat::R8G8B8_SRGB)
        {
                return conv_bgra_to_rgb(pixels);
        }
        if (to_format == image::ColorFormat::R8G8B8A8_SRGB)
        {
                return conv_bgra_to_rgba(pixels);
        }

        error("Unsupported format conversion from BGRA to " + image::format_to_string(to_format));
}
}
