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

#include <cstring>
#include <vector>

namespace image
{
template <size_t N>
void flip_vertically(Image<N>* image)
{
        if (image->size[0] < 1 || image->size[1] < 1)
        {
                error("Error image size");
        }

        size_t pixel_size = format_pixel_size_in_bytes(image->color_format);
        size_t row_size = pixel_size * image->size[0];
        size_t size_2d = row_size * image->size[1];
        if (image->pixels.size() % size_2d != 0)
        {
                error("Error image pixels size");
        }

        std::vector<std::byte> row(row_size);
        size_t r1_end = row_size * (image->size[1] / 2);
        size_t r2_init = row_size * (image->size[1] - 1);
        for (size_t offset = 0; offset < image->pixels.size(); offset += size_2d)
        {
                for (size_t r1 = offset, r2 = offset + r2_init; r1 < offset + r1_end; r1 += row_size, r2 -= row_size)
                {
                        std::memcpy(row.data(), &(image->pixels)[r1], row_size);
                        std::memcpy(&(image->pixels)[r1], &(image->pixels)[r2], row_size);
                        std::memcpy(&(image->pixels)[r2], row.data(), row_size);
                }
        }
}
}
