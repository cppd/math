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

#include "initial_image.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/image/conversion.h>
#include <src/image/format.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace ns::gui::painter_window
{
std::vector<std::byte> make_initial_image(const std::vector<int>& screen_size, const image::ColorFormat color_format)
{
        constexpr std::array<std::uint8_t, 4> LIGHT = {100, 150, 200, 255};
        constexpr std::array<std::uint8_t, 4> DARK = {0, 0, 0, 255};

        const std::size_t pixel_size = image::format_pixel_size_in_bytes(color_format);

        std::vector<std::byte> light;
        std::vector<std::byte> dark;
        image::format_conversion(
                image::ColorFormat::R8G8B8A8_SRGB, std::as_bytes(std::span(LIGHT)), color_format, &light);
        image::format_conversion(
                image::ColorFormat::R8G8B8A8_SRGB, std::as_bytes(std::span(DARK)), color_format, &dark);
        ASSERT(pixel_size == light.size());
        ASSERT(pixel_size == dark.size());

        const int slice_count = multiply_all<long long>(screen_size) / screen_size[0] / screen_size[1];

        std::vector<std::byte> image(pixel_size * multiply_all<long long>(screen_size));

        std::size_t index = 0;
        for (int i = 0; i < slice_count; ++i)
        {
                for (int y = 0; y < screen_size[1]; ++y)
                {
                        for (int x = 0; x < screen_size[0]; ++x)
                        {
                                const std::byte* const pixel = ((x + y) & 1) ? light.data() : dark.data();
                                std::memcpy(&image[index], pixel, pixel_size);
                                index += pixel_size;
                        }
                }
        }

        ASSERT(index == image.size());

        return image;
}
}
