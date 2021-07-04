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

#include "depth.h"

#include "conversion.h"
#include "format.h"

#include <src/com/error.h>

namespace ns::image
{
template <std::size_t N>
[[nodiscard]] Image<N> convert_to_8_bit(const Image<N>& image)
{
        ColorFormat color_format = [&]()
        {
                switch (image.color_format)
                {
                case ColorFormat::R8_SRGB:
                case ColorFormat::R8G8B8_SRGB:
                case ColorFormat::R8G8B8A8_SRGB:
                case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                        return image.color_format;
                case ColorFormat::R16:
                case ColorFormat::R32:
                        return ColorFormat::R8_SRGB;
                case ColorFormat::R16G16B16:
                case ColorFormat::R16G16B16_SRGB:
                case ColorFormat::R32G32B32:
                        return ColorFormat::R8G8B8_SRGB;
                case ColorFormat::R16G16B16A16:
                case ColorFormat::R16G16B16A16_SRGB:
                case ColorFormat::R32G32B32A32:
                        return ColorFormat::R8G8B8A8_SRGB;
                case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                        return ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED;
                }
                unknown_color_format_error(image.color_format);
        }();

        if (color_format != image.color_format)
        {
                Image<N> result;
                result.size = image.size;
                result.color_format = color_format;
                format_conversion(image.color_format, image.pixels, result.color_format, &result.pixels);
                return result;
        }

        return image;
}

template Image<2> convert_to_8_bit(const Image<2>&);
template Image<3> convert_to_8_bit(const Image<3>&);
template Image<4> convert_to_8_bit(const Image<4>&);
template Image<5> convert_to_8_bit(const Image<5>&);
}