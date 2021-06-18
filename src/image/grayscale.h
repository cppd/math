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

#pragma once

#include "image.h"

namespace ns::image
{
void make_grayscale(ColorFormat color_format, const std::span<std::byte>& bytes);
std::vector<std::byte> convert_to_r_component_format(ColorFormat color_format, const std::span<const std::byte>& bytes);

template <std::size_t N>
Image<N> convert_to_r_component_format(const Image<N>& image)
{
        Image<N> result;

        result.color_format = [&]()
        {
                switch (image.color_format)
                {
                case ColorFormat::R8_SRGB:
                case ColorFormat::R16:
                case ColorFormat::R32:
                case ColorFormat::R16G16B16_SRGB:
                case ColorFormat::R16G16B16A16_SRGB:
                case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                        error("Unsupported image format " + format_to_string(image.color_format)
                              + " for converting image to R component format");
                case ColorFormat::R8G8B8_SRGB:
                case ColorFormat::R8G8B8A8_SRGB:
                        return ColorFormat::R8_SRGB;
                case ColorFormat::R16G16B16:
                case ColorFormat::R16G16B16A16:
                        return ColorFormat::R16;
                case ColorFormat::R32G32B32:
                case ColorFormat::R32G32B32A32:
                        return ColorFormat::R32;
                }
                unknown_color_format_error(image.color_format);
        }();

        result.size = image.size;
        result.pixels = convert_to_r_component_format(image.color_format, image.pixels);

        return result;
}
}
