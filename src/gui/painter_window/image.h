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

#include <src/color/color.h>
#include <src/com/alg.h>
#include <src/com/error.h>

#include <array>
#include <vector>

namespace gui::painter_window
{
template <size_t N>
std::vector<std::byte> make_bgra_image(const std::array<int, N>& screen_size)
{
        constexpr Srgb8 LIGHT = Srgb8(100, 150, 200);
        constexpr Srgb8 DARK = Srgb8(0, 0, 0);

        constexpr std::array<unsigned char, 4> LIGHT_BGR = {LIGHT.blue, LIGHT.green, LIGHT.red, 0};
        constexpr std::array<unsigned char, 4> DARK_BGR = {DARK.blue, DARK.green, DARK.red, 0};

        const int count = multiply_all<long long>(screen_size)
                          / (static_cast<long long>(screen_size[0]) * static_cast<long long>(screen_size[1]));

        std::vector<std::byte> image(4 * multiply_all<long long>(screen_size));

        size_t index = 0;
        for (int i = 0; i < count; ++i)
        {
                for (int y = 0; y < screen_size[1]; ++y)
                {
                        for (int x = 0; x < screen_size[0]; ++x)
                        {
                                std::memcpy(&image[index], ((x + y) & 1) ? LIGHT_BGR.data() : DARK_BGR.data(), 4);
                                index += 4;
                        }
                }
        }

        ASSERT(index == image.size());

        return image;
}

}
