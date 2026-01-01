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

#include "flip.h"

#include "image.h"

#include <src/com/error.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <cstring>
#include <vector>

namespace ns::image
{
template <std::size_t N>
void flip_vertically(Image<N>* const image)
{
        if (image->size[0] < 1 || image->size[1] < 1)
        {
                error("Error image size");
        }

        const std::size_t pixel_size = format_pixel_size_in_bytes(image->color_format);
        const std::size_t row_size = pixel_size * image->size[0];
        const std::size_t size_2d = row_size * image->size[1];
        if (image->pixels.size() % size_2d != 0)
        {
                error("Error image pixels size");
        }

        std::vector<std::byte> row(row_size);
        const std::size_t r1_end = row_size * (image->size[1] / 2);
        const std::size_t r2_init = row_size * (image->size[1] - 1);
        for (std::size_t offset = 0; offset < image->pixels.size(); offset += size_2d)
        {
                std::size_t r1 = offset;
                std::size_t r2 = offset + r2_init;
                for (; r1 < offset + r1_end; r1 += row_size, r2 -= row_size)
                {
                        std::memcpy(row.data(), &(image->pixels)[r1], row_size);
                        std::memcpy(&(image->pixels)[r1], &(image->pixels)[r2], row_size);
                        std::memcpy(&(image->pixels)[r2], row.data(), row_size);
                }
        }
}

#define TEMPLATE(N) template void flip_vertically(Image<(N)>*);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
