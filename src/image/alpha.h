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

#pragma once

#include "image.h"

#include <src/com/error.h>

namespace image
{
std::vector<std::byte> add_alpha(ColorFormat color_format, const std::span<const std::byte>& bytes, float alpha);

template <size_t N>
void add_alpha(image::Image<N>* image, float alpha)
{
        if (image->color_format == image::ColorFormat::R8G8B8_SRGB)
        {
                image->pixels = image::add_alpha(image->color_format, image->pixels, alpha);
                image->color_format = image::ColorFormat::R8G8B8A8_SRGB;
        }
        else if (image->color_format == image::ColorFormat::R16G16B16)
        {
                image->pixels = image::add_alpha(image->color_format, image->pixels, alpha);
                image->color_format = image::ColorFormat::R16G16B16A16;
        }
        else if (image->color_format == image::ColorFormat::R32G32B32)
        {
                image->pixels = image::add_alpha(image->color_format, image->pixels, alpha);
                image->color_format = image::ColorFormat::R32G32B32A32;
        }
        else
        {
                error("Unsupported image format " + format_to_string(image->color_format) + " for adding alpha");
        }
}

std::vector<std::byte> delete_alpha(image::ColorFormat color_format, const std::span<const std::byte>& bytes);
}
