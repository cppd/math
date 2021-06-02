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

#include <src/com/error.h>
#include <src/numerical/vec.h>

namespace ns::image
{
std::vector<std::byte> add_alpha(ColorFormat color_format, const std::span<const std::byte>& bytes, float alpha);
std::vector<std::byte> delete_alpha(ColorFormat color_format, const std::span<const std::byte>& bytes);
void blend_alpha(ColorFormat* color_format, const std::span<std::byte>& bytes, Vector<3, float> rgb);
void set_alpha(ColorFormat color_format, const std::span<std::byte>& bytes, float alpha);

template <std::size_t N>
Image<N> add_alpha(const Image<N>& image, float alpha)
{
        Image<N> result;

        if (image.color_format == ColorFormat::R8G8B8_SRGB)
        {
                result.color_format = ColorFormat::R8G8B8A8_SRGB;
        }
        else if (image.color_format == ColorFormat::R16G16B16)
        {
                result.color_format = ColorFormat::R16G16B16A16;
        }
        else if (image.color_format == ColorFormat::R32G32B32)
        {
                result.color_format = ColorFormat::R32G32B32A32;
        }
        else
        {
                error("Unsupported image format " + format_to_string(image.color_format) + " for adding alpha");
        }

        result.size = image.size;
        result.pixels = add_alpha(image.color_format, image.pixels, alpha);

        return result;
}

template <std::size_t N>
Image<N> delete_alpha(const Image<N>& image)
{
        Image<N> result;

        if (image.color_format == ColorFormat::R8G8B8A8_SRGB)
        {
                result.color_format = ColorFormat::R8G8B8_SRGB;
        }
        else if (image.color_format == ColorFormat::R16G16B16A16)
        {
                result.color_format = ColorFormat::R16G16B16;
        }
        else if (image.color_format == ColorFormat::R32G32B32A32)
        {
                result.color_format = ColorFormat::R32G32B32;
        }
        else
        {
                error("Unsupported image format " + format_to_string(image.color_format) + " for deleting alpha");
        }

        result.size = image.size;
        result.pixels = delete_alpha(image.color_format, image.pixels);

        return result;
}
}
