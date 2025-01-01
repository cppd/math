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

#pragma once

#include "format.h"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace ns::image
{
template <std::size_t N>
struct Image final
{
        std::array<int, N> size;
        ColorFormat color_format;
        std::vector<std::byte> pixels;
};

template <std::size_t N>
struct ImageView final
{
        std::array<int, N> size;
        ColorFormat color_format;
        std::span<const std::byte> pixels;

        explicit ImageView(const Image<N>& image)
                : size(image.size),
                  color_format(image.color_format),
                  pixels(image.pixels)
        {
        }

        ImageView(
                const std::array<int, N>& size,
                const ColorFormat color_format,
                const std::span<const std::byte> pixels)
                : size(size),
                  color_format(color_format),
                  pixels(pixels)
        {
        }
};
}
